#include "tcp_connection.hh"

#include <iostream>
#include <limits>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

namespace {
constexpr size_t LINGER_MULTIPLIER = 10;  // lab: linger for 10×RTO
}  // namespace

// Move all segments produced by the sender into the connection's outbound queue.
//
// This helper is called after any event that might have changed sender output
// or receiver state (write(), segment_received(), tick(), etc.).
void TCPConnection::push_sender_segments() {
	auto& sender = _sender.segments_out();
	while (!sender.empty()) {
		TCPSegment segment = std::move(sender.front());
		sender.pop();

		// The TCPSender does not know the receiver's ackno/window, so TCPConnection
		// must fill in:
		//   - win: receiver's current advertised window (clamped to uint16_t)
		//   - ack + ackno: only if receiver has an ackno (i.e., after aSYN received)
		const size_t win = min(_receiver.window_size(), static_cast<size_t>(numeric_limits<uint16_t>::max()));
		segment.header().win = static_cast<uint16_t>(win);
		if (_receiver.ackno().has_value()) {
			segment.header().ack = true;
			segment.header().ackno = _receiver.ackno().value();
		}
		_segments_out.push(segment);
	}
}

// Send a single RST segment and transition to an inactive/error state.
//
// Lab rule: any unclean shutdown (timeout / destructor / excessive retransmissions)
// should send a RST so the peer can quickly fail the connection.
// We generate an "empty" segment from the sender (so seqno is correct) and then
// set rst=true, plus attach ack/win like any other outgoing segment.
void TCPConnection::send_reset_segment() {
	// Discard any pending (non-RST) segments from the sender.
	auto& sender = _sender.segments_out();
	while (!sender.empty()) {
		sender.pop();
	}
	// Force the sender to generate an empty segment with a correct seqno.
	_sender.send_empty_segment();
	if (!sender.empty()) {
		TCPSegment segment = sender.front();
		sender.pop();

		segment.header().rst = true;
		const size_t win = min(_receiver.window_size(), static_cast<size_t>(numeric_limits<uint16_t>::max()));
		segment.header().win = static_cast<uint16_t>(win);
		if (_receiver.ackno().has_value()) {
			segment.header().ack = true;
			segment.header().ackno = _receiver.ackno().value();
		}
		_segments_out.push(segment);
    }
	// An outgoing RST is an unclean shutdown.
	_sender.stream_in().set_error();
	_receiver.stream_out().set_error();
	_linger_after_streams_finish = false;
	_active = false;
}

// Determine if the connection can be considered closed (active() becomes false).
//
// Clean shutdown prerequisites (handout summary):
//   1) inbound stream fully assembled and ended
//   2) outbound stream ended and FIN sent
//   3) all outbound bytes (incl. FIN) acknowledged
//   4) if we might need to keep ACKing (linger), wait 10×RTO after last segment
//      unless _linger_after_streams_finish has been disabled.
void TCPConnection::check_for_clean_shutdown() {
	if (!_active) {
		return;
	}

	// Inbound side: stream has logically ended and all bytes are reassembled.
	// Use input_ended() (not eof()) because TCP close should not depend on whether the app
	// has already drained the received buffer; it only depends on FIN being reached with no gaps.
	const bool inbound_done = _receiver.stream_out().input_ended() && _receiver.unassembled_bytes() == 0;

	// Outbound side: app has ended its write stream and the sender has consumed all buffered data,
	// so it can (and did) send FIN. Also require all outstanding seqno-space (incl. FIN) to be ACKed.
	// Use eof() (not input_ended()) to ensure the sender has actually pulled everything from the stream.
	const bool outbound_done = _sender.stream_in().eof() && _sender.bytes_in_flight() == 0;

	if (!inbound_done || !outbound_done) {
		return;
	}

	if (!_linger_after_streams_finish) {
		_active = false;
		return;
	}

	if (_time_since_last_segment_received >= LINGER_MULTIPLIER * _cfg.rt_timeout) {
		_active = false;
	}
}

size_t TCPConnection::remaining_outbound_capacity() const {
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const {
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const {
    return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const {
	return _time_since_last_segment_received;
}

// Process a segment from the peer.
//
// Responsibilities:
//   - RST: immediately abort and mark both streams error
//   - otherwise deliver to receiver, and pass ACK info to sender
//   - possibly generate an empty ACK segment in response to "useful" inbound data
//     (or keep-alive probes), even if sender has nothing new to send
//   - after processing, attach ack/win to any outgoing segments and queue them.
void TCPConnection::segment_received(const TCPSegment &seg) {
	if (!_active) {
		return;
	}

	_time_since_last_segment_received = 0;

    if (seg.header().rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _linger_after_streams_finish = false;
        _active = false;
        return;
    }
    _receiver.segment_received(seg);

	// If we are just listening, ignore irrelevant noise packets ---
	// If we have neither sent nor received a SYN, it means we have a LISTEN
    if (!_receiver.ackno().has_value() && _sender.next_seqno_absolute() == 0) {
        return;
    }

    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }

	// Passive close rule: if the peer finished its stream first, then it knows it has
    // received all our ACKs (because it initiated the close). In that case we can
    // disable linger and close immediately once our own outbound bytes are fully ACKed.
	//
	// _receiver.stream_out().input_ended()：peer finish writing first or close first, our inbound ended
	// !_sender.stream_in.eof(): we still keep sending
	if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
		_linger_after_streams_finish = false;
	}

	// Opportunity to send data/SYN/FIN now that receiver state (ackno/win) may have advanced.
    _sender.fill_window();

	// Respond to keep-alive probes:
    // A keep-alive is an empty segment whose seqno == (our ackno - 1).
    // TCPConnection should reply with an empty ACK reflecting current ack/win.
    if (_receiver.ackno().has_value() && seg.length_in_sequence_space() == 0
    && seg.header().seqno == _receiver.ackno().value() - 1) {
        _sender.send_empty_segment();
    }

	// If the incoming segment occupied sequence numbers (SYN/data/FIN),
    // the peer may be expecting an ACK update even when we have nothing to send.
    // The lab requires: if seg.length_in_sequence_space() > 0 and sender produced
    // no output, send an empty ACK segment.
	if (_receiver.ackno().has_value() && seg.length_in_sequence_space() > 0
	&& _sender.segments_out().empty()) {
		_sender.send_empty_segment();
	}

	push_sender_segments();
	check_for_clean_shutdown();
}

bool TCPConnection::active() const {
    return _active;
}

// Write application data into the sender's outbound ByteStream, then let the sender
// fill the window and emit any new segments (data/FIN).
size_t TCPConnection::write(const string &data) {
	if (!_active) {
        return 0; // Don't write to inactive connection
    }
	const size_t write_bytes = _sender.stream_in().write(data);
	_sender.fill_window();
	push_sender_segments();
	check_for_clean_shutdown();
	return write_bytes;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
	if (!_active) {
		return;
	}

	_time_since_last_segment_received += ms_since_last_tick;
	_sender.tick(ms_since_last_tick);
	// Too many consecutive retransmissions => give up and reset.
	if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
		send_reset_segment();
		return;
	}
	push_sender_segments();
	check_for_clean_shutdown();
}

// Application is done writing: end the outbound stream, which eventually causes
// the sender to transmit a FIN when window permits.
void TCPConnection::end_input_stream() {
	if (!_active) {
        return; // Don't process if inactive
    }
    _sender.stream_in().end_input();
	_sender.fill_window();
	push_sender_segments();
	check_for_clean_shutdown();
}

// Active open: ask the sender to fill its window, which will send SYN as needed.
void TCPConnection::connect() {
    _sender.fill_window(); // will send SYN
	push_sender_segments();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
			send_reset_segment();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
