Lab 2 Writeup
=============

# Translating between 64-bit indexes and 32-bit seqnos
In this lab, we implemented a simple translation layer that allows us to use 64-bit indexes 
while still maintaining compatibility with 32-bit sequence numbers. 

For `wrap()`, the implementation is easy, and for `unwrap()`, we need to consider the wrap-around behavior of sequence numbers.
Here is what I do in my implementation for `unwrap()``:
* First, calculate the relative index offset = (n - isn) mod 2^32.
* Construct a candidate base of values with the same wrap period using the high 32 bits of the checkpoint.
* Then compare the base, base+2^32, and base-2^32 (if they exist), and select the one closest to the checkpoint.

# Implementing the TCP receiver
The `segment_received` function is responsible for processing incoming TCP segments. Here is what I do in my implementation:
* If we haven't seen a SYN yet, ignore everything until the first SYN arrives. And record the ISN from the SYN segment if it arrives.
* Convert the segment's (wrapping) sequence number to an absolute sequence number.
* Translate absolute seqno (SYN is abs=0) into stream index (0-based payload index). Notice if the segment has a SYN flag, 
we should set the stream index to 0. Otherwise, the stream index is abs_seqno - 1 
(because SYN consumes one sequence number).
* Push payload and/or EOF marker (FIN) to the reassembler.

The `ackno()` function returns the next expected sequence number, which is the index of the first byte that has not been received yet.
* If we haven't seen a SYN yet, return an empty optional.
* Otherwise, return the wrapping sequence number corresponding to the next expected byte index 
(which is the index of the first byte that has not been received yet). Notice if the input stream has ended 
(i.e., we have received a FIN), the next expected byte index should be the index of the first byte that has not been received yet plus one 
(because FIN also consumes one sequence number).

The `window_size()` function returns the size of the receiver's window, which is the amount of space left in the reassembler's buffer.
* notice the definition: the size of the window of acceptable indices 
that the receiver is willing to accept. It's the distance between
the first unassembled and the first unacceptable index.
* In implementation: it's the capacity minus the number of bytes that the
TCPReceiver is holding in the byte stream.