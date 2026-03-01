#include "tcp_sender.hh"

#include "tcp_config.hh"
#include "wrapping_integers.hh"

#include <algorithm>
#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) 
    , _timer(retx_timeout)
    {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _acked_seqno; }

void TCPSender::fill_window() {
    // Lab 3: "If the receiver has announced a window size of zero, act like the window size is one."
    const uint16_t effective_win = (_window_size == 0) ? 1 : _window_size;
    size_t flight = bytes_in_flight();
    size_t available = (effective_win > flight) ? (effective_win - flight) : 0;

    // Send SYN if not yet sent (needs 1 byte of window; assume 1 before first ACK per FAQ)
    if (_next_seqno == 0) {
        if (available == 0) {
            return;
        }
        TCPSegment seg;
        seg.header().syn = true;
        seg.header().seqno = wrap(_next_seqno, _isn);
        _segments_out.push(seg);
        _outstanding_segments.push_back(make_pair(_next_seqno, seg));
        _next_seqno = 1;
        available -= 1; // SYN occupies one sequence number
        if (!_timer.is_running()) {
            _timer.restart();
        }
    }

    // Send data and/or FIN until window is full or stream is empty
    while (available > 0) {
        size_t payload_size = min({available, static_cast<size_t>(TCPConfig::MAX_PAYLOAD_SIZE), _stream.buffer_size()});
        bool send_fin = false;
        if (_stream.input_ended() && !_fin_sent) {
            send_fin = (payload_size + 1 <= available);  // FIN occupies one sequence number
        }

        // no more data to send and no FIN to send
        if (payload_size == 0 && !send_fin) {
            break;
        }

        TCPSegment seg;
        seg.header().seqno = wrap(_next_seqno, _isn);
        if (payload_size > 0) {
            seg.payload() = Buffer(_stream.read(payload_size));
        }
        seg.header().fin = send_fin;
        if (send_fin) {
            _fin_sent = true;
        }

        size_t seg_len = seg.length_in_sequence_space();
        _segments_out.push(seg);
        _outstanding_segments.push_back(make_pair(_next_seqno, seg));
        _next_seqno += seg_len;
        if (available > seg_len) {
            available -= seg_len;
        } else {
            available = 0;
        }
        

        if (!_timer.is_running()) {
            _timer.restart();
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ack = unwrap(ackno, _isn, _next_seqno);
    // Completely ignore impossible ACKs
    if (abs_ack > _next_seqno) {
        return;
    }

    // Outdated/duplicate ACKs might carry a new window size (window update). 
    // Record the new window size and see if we can send more data.
    if (abs_ack <= _acked_seqno) {
        _window_size = window_size;
        fill_window();
        return;
    }

    _acked_seqno = abs_ack;

    while (!_outstanding_segments.empty()) {
        auto& front = _outstanding_segments.front();
        uint64_t last = front.first + front.second.length_in_sequence_space();
        if (last <= abs_ack) {
            _outstanding_segments.pop_front();
        } else {
            break;
        }
    }

    // reset timer and consecutive retransmissions
    _timer.set_rto(_initial_retransmission_timeout);
    _consecutive_retransmissions = 0;

    if (!_outstanding_segments.empty()) {
        _timer.restart();
    } else {
        _timer.stop();
    }

    _window_size = window_size;
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (!_timer.is_running()) {
        return;
    }
    _timer.tick(ms_since_last_tick);
    if (_timer.check_time_out()) {
        if (!_outstanding_segments.empty()) {
            TCPSegment seg = _outstanding_segments.front().second;
            _segments_out.push(seg);

            if (_window_size != 0) {
                _consecutive_retransmissions++;
                _timer.set_rto(_timer.get_rto() * 2);
            }
            _timer.restart();
        } else {
            _timer.stop();
        }
    } 
 }

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}
