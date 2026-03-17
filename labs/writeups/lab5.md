# Lab 5 Writeup

## The Address Resolution Protocol

In this lab, we will implement a network interface: the bridge
between Internet datagrams that travel the world, and link-layer Ethernet frames that travel one hop. Specifically, we will implement Address Resolution Protocol (ARP). The main function is to find and cache the Ethernet address for each next-hop IP address. This lab is relatively easy and just follow the documents' instructions.

`send_datagram`

- This method is called when the caller (e.g., your TCPConnection or a router) wants to
send an outbound Internet (IP) datagram to the next hop. 
- If the destination Ethernet address is already known, send it right away. 
  - Create an Ethernet frame (with type = `EthernetHeader::TYPE IPv4`), set the payload to be the serialized datagram, and set the source and destination addresses
- If the destination Ethernet address is unknown
  - If the network interface already sent an ARP request about the same IP address in the last five seconds, don’t send a second request—just wait for a reply to the first one
  - Otherwise, broadcast an ARP request for the next hop’s Ethernet address, and **add the datagram to the queue of datagrams waiting for ARP resolution**, therefore it can be sent after the ARP reply is received
  
`recv_frame`
- This method is called when an Ethernet frame arrives from the network. 
- Ignore any frames not destined for the network interface (meaning, the Ethernet destination is either the broadcast address or the interface’s own Ethernet address stored in the ethernet address member variable).
- If the inbound frame is `IPv4`, parse the payload as an `InternetDatagram` and, if successful (meaning the parse() method returned `ParseResult::NoError`), return the resulting `InternetDatagram` to the caller.
- If the inbound frame is `ARP`
  - Parse the payload as an ARPMessage and, if successful,
remember the mapping between the sender’s IP address and Ethernet address for 30 seconds.
  - **If there are any datagrams waiting for this IP address, send them now**.
  - If it’s an ARP request asking for our IP address, send an appropriate ARP reply.

`tick`
- This is called as time passes. 
- Expire any IP-to-Ethernet mappings that have expired, i.e. remove any ARP entry that has been stored for 30 seconds or more.
- **Remove any ARP request that has been pending for 5 seconds or more**. Note in this lab, for simplicity, if an ARP request times out (no response received within 5 seconds), there is no need to resend it or even return an ICMP packet with the 'host unreachable' message.

