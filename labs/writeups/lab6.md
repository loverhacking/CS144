# Lab 6 Writeup
## Implementing the Router
In this lab, the job is to implement an IP router on top of existing
`NetworkInterface`. A router has several network interfaces, and can receive Internet datagrams on any of them. 
The router’s job is to forward the datagrams it gets according to the
routing table: a list of rules that tells the router, for any given datagram,
* What interface to send it out
* The IP address of the next hop

The main job is to implement a router that can figure out these two things for any given datagram.

This lab is relatively more easy, and just follow the instructions in the lab manual. 

`add_route`
* This method adds a route to the routing table.
* create a `Route` struct to store the route information, 
and use a route table (e.g., a vector) to store all the routes.
* sort the route table based on the prefix length in descending order, 
so that we can find the longest prefix match when forwarding datagrams in a first try.

`route_one_datagram`
* This method needs to route one datagram to the
  next hop, out the appropriate interface. 
* The router decrements the datagram’s TTL (time to live). If the TTL was zero already,
  or hits zero after the decrement, the router should drop the datagram
* The Router searches the routing table to find the routes that match the datagram’s
  destination address.
  * can use a `mask` to check if the datagram’s destination address matches the route’s prefix. 
  The way to create `mask` is to left shift the result by (32 - prefix length) if prefix length is nonzero.
* If there are no matching routes, the router should drop the datagram. Otherwise, the router sends the modified datagram on the appropriate interface
  to the appropriate next hop.
  * Note, even in the real world, 
  not every router will send an ICMP message back to the source if the router has no route to the destination, 
  or if the TTL hits zero.
