Lab 4 Writeup
=============

## The TCP connection
In this lab, we will finish building a working TCP implementation. We’ve already done most
of the work to get there: we’ve implemented the sender and the receiver. 
This week is to “wire them up” together into one object (a TCPConnection) and handle some
housekeeping tasks that are global to the connection.

`segment_received`
* if `rst` flag is set, sets both the inbound and outbound streams to the error
  state and kills the connection permanently
* gives the segment to the TCPReceiver so it can inspect the fields it cares about on
  incoming segments: seqno, syn , payload, and fin .
* if we are in the LISTEN state(neither sent nor received a SYN), we ignore the segment
* if the `ack` flag is set, tells the TCPSender about the fields it cares about on incoming
  segments: ackno and window size.
* if the peer finished its stream first, then it knows it has
  received all our ACKs (because it initiated the close). In that case we can
  disable linger and close immediately once our own outbound bytes are fully ACKed. (**Passive close rule**)
* send data/SYN/FIN now that receiver state (ackno/win) may have advanced
* Respond to **keep-alive probes**: A keep-alive is an empty segment whose seqno == (our ackno - 1).
  TCPConnection should reply with an empty ACK reflecting current ack/win.
* If the incoming segment occupied sequence numbers (SYN/data/FIN),
  the peer may be expecting an ACK update even when we have nothing to send.
  the lab requires: if seg.length_in_sequence_space() > 0 and sender produced
  no output, send an empty ACK segment.
* push segments produced by the sender into the connection's outbound queue and check 
  whether we have clean shutdown conditions.

`write`
* If the connection is still alive, write data to the outbound stream and then try to send
  segments to the peer. If the connection is not alive, do nothing.

`tick`
* If the connection is still alive, call the sender's tick method to check for retransmissions 
and then try to send segments to the peer. If the connection is not alive, do nothing.
* If we have too many consecutive retransmissions, send a segment with the rst flag set.

`end_input_stream`
* If the connection is still alive, end the outbound stream and then try to send segments to the peer. 
If the connection is not alive, do nothing.

`connect`
* ask the sender to fill its window, which will send SYN as needed.

I also write 3 helpful functions:

`push_sender_segments`
* Move all segments produced by the sender into the connection's outbound queue.
* This helper is called after any event that might have changed sender output
  or receiver state (write(), segment_received(), tick(), etc.).
* The TCPSender does not know the receiver's ackno/window, so TCPConnection
  must fill in:
    - win: receiver's current advertised window (clamped to uint16_t)
    - ack + ackno: only if receiver has an ackno (i.e., after aSYN received)

`send_reset_segment`
* Any unclean shutdown (timeout / destructor / excessive retransmissions)
  should send a RST so the peer can quickly fail the connection.
* We generate an "empty" segment from the sender (so seqno is correct) and then
  set rst=true, plus attach ack/win like any other outgoing segment.
  Note we clear all the segments in TCPsender before we send the reset, 
  so that we don't accidentally send any data that was pending.
* An outgoing RST is an unclean shutdown, so we also set both streams to the error state 
  and mark the connection as dead.

`check_for_clean_shutdown`
* If the connection is still alive, check whether we have clean shutdown conditions:
  - inbound stream fully assembled and ended
  - outbound stream ended and FIN sent
  - all outbound bytes (incl. FIN) acknowledged
  - if we might need to keep ACKing (linger), wait 10×RTO after last segment
  unless `_linger_after_streams_finish` has been disabled.

## Performance

CPU-limited throughput                : 1.79 Gbit/s

CPU-limited throughput with reordering: 1.77 Gbit/s