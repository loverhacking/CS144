Lab 3 Writeup
=============

## Implementing the TCP sender
In this lab, we implemented a simple TCP sender that can send data to a TCP receiver.  TCPSender’s responsibility is to:
* Keep track of the receiver’s window (processing incoming acknos and window sizes)
* Fill the window when possible, by reading from the ByteStream, creating new TCP
segments (including SYN and FIN flags if needed), and sending them. The sender
should keep sending segments until either the window is full or the ByteStream is
empty.
* Keep track of which segments have been sent but not yet acknowledged by the receiver—
we call these “outstanding” segments
* Re-send outstanding segments if enough time passes since they were sent, and they
haven’t been acknowledged yet

First, I implemented the `Timer` class, which is responsible for keeping track of the time since a segment was sent. 
It has methods to restart, stop, set rto value and check if the timer has been running and expired.

`fill window`
* send SYN segment if not sent yet
* while the window is not full and there is data to send, 
  * create a new segment with the next byte of data from the ByteStream
  * set the appropriate flags (SYN, FIN)
  * record the segment as outstanding
  * If the timer is not running, start the timer

`ack_received`
* ignore the ack if it is for a segment that has not been sent yet
* duplicate ACKs might carry a new window size (window update). 
Record the new window size and see if we can send more data.
* remove all segments from the list of outstanding segments that are acknowledged by this ACK
* reset timer rto and consecutive retransmissions.
* restart the timer if there are still outstanding segments and stop the timer if not.
* update the window size and see if we can send more data.

`tick`
* if the timer is not running, do nothing
* if the timer has expired, 
  * retransmit the oldest outstanding segment if there are any, and stop the timer if not
  * if window size is nonzero, increment the number of consecutive retransmissions
  , and double the timer rto
  * restart the timer

Note:
* we only set one timer, and it is for the oldest outstanding segment. 
* use `pair` to record the sequence number and the segment for each outstanding segment, 
so that we can easily check if an ACK is for an outstanding segment and remove it from the list of outstanding segments when we receive an ACK for it.
* record the (absolute) sequence number that the receiver has acknowledged
* send FIN when
  * we have not sent any FIN yet
  * the input ByteStream has no more data to read and is at eof state
  * the window has space for at least one more byte (i.e. the FIN flag takes up one byte of the window)
