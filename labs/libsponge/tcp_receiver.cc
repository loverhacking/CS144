#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
	const TCPHeader &hdr = seg.header();
	// If we haven't seen a SYN yet, ignore everything until the first SYN arrives.
    if (!_isn.has_value()) {
        if (hdr.syn) {
            _isn = hdr.seqno;
        } else {
            // Ignore segments before the SYN
            return;
        }
    }

	// Convert the segment's (wrapping) sequence number to an absolute sequence number.
    // Checkpoint should be close to what we expect next:
    // next abs seqno ≈ bytes_written + 1 (SYN consumes one seqno).
	const uint64_t checkPoint = _reassembler.stream_out().bytes_written() + 1;
    const uint64_t absSeqno = unwrap(hdr.seqno, *_isn, checkPoint);

	uint64_t stream_index = 0;
	if (hdr.syn) {
        // seqno points at SYN; payload begins at abs=1 => stream index 0.
        stream_index = absSeqno;
    } else {
        if (absSeqno < 1) {
            // Payload can't begin before abs seqno 1 (SYN).
            return;
        }
        // Payload begins at abs=absSeqno; stream index is absSeqno - 1 (SYN).
        stream_index = absSeqno - 1;
    }
	// Push payload and/or EOF marker (FIN) to the reassembler.
    // If FIN is set, it means the stream ends right after this payload.
	_reassembler.push_substring(seg.payload().copy(), stream_index, hdr.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
	if (!_isn.has_value()) {
        return nullopt;
    }

    uint64_t ackAbs = _reassembler.stream_out().bytes_written() + 1; // next byte is what we ACK
	if (_reassembler.stream_out().input_ended()) {
        ackAbs += 1; // FIN consumes one seqno
	}
    return wrap(ackAbs, *_isn);
}

size_t TCPReceiver::window_size() const {
	const size_t stored = _reassembler.stream_out().buffer_size();
    return (_capacity > stored) ? (_capacity - stored) : 0;
}
