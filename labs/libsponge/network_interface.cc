#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
	if (_ip_to_ethernet.count(next_hop_ip)) {
		// Create an Ethernet frame (with type = EthernetHeader::TYPE IPv4), set the payload to
        // be the serialized datagram, and set the source and destination addresses.
        EthernetFrame frame;
        frame.header().type = EthernetHeader::TYPE_IPv4;
        frame.header().src = _ethernet_address;
        frame.header().dst = _ip_to_ethernet[next_hop_ip];
        frame.payload() = dgram.serialize();
        _frames_out.push(frame);
    } else {
		// If the destination Ethernet address is unknown, broadcast an ARP request for the
		// next hop’s Ethernet address, and queue the IP datagram so it can be sent after
		// the ARP reply is received.

        // Add the datagram to the queue of datagrams waiting for ARP resolution.
        _waiting_datagrams[next_hop_ip].push(dgram);

		if (_pending_arp_requests.count(next_hop_ip) && _pending_arp_request_time[next_hop_ip] <= 5000) {
            // Already sent an ARP request for this IP address, and it's still within the 5 second timeout, 
            // so just wait for the reply.
            return;
		}

        // Add the IP address to the pending ARP requests and set the request time to 0.
		_pending_arp_requests.insert(next_hop_ip);
        _pending_arp_request_time[next_hop_ip] = 0;
        
        ARPMessage arp_msg;
        arp_msg.opcode = ARPMessage::OPCODE_REQUEST;
        arp_msg.sender_ethernet_address = _ethernet_address;
        arp_msg.sender_ip_address = _ip_address.ipv4_numeric();
        arp_msg.target_ethernet_address = {};
        arp_msg.target_ip_address = next_hop_ip;

        EthernetFrame frame;
        frame.header().type = EthernetHeader::TYPE_ARP;
        frame.header().src = _ethernet_address;
        frame.header().dst = ETHERNET_BROADCAST;
        frame.payload() = BufferList(arp_msg.serialize());
        _frames_out.push(frame);
    }


}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    if (frame.header().dst != _ethernet_address && frame.header().dst != ETHERNET_BROADCAST) {
        // Frame is not intended for this interface, so ignore it.
        return nullopt;
    }

    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
		// If the inbound frame is IPv4, parse the payload as an InternetDatagram and,
		// if successful (meaning the parse() method returned ParseResult::NoError),
		// return the resulting InternetDatagram to the caller.
        InternetDatagram dgram;
        if (dgram.parse(frame.payload()) == ParseResult::NoError) {
            return dgram;
        }
    } else if (frame.header().type == EthernetHeader::TYPE_ARP) {
		// If the inbound frame is ARP, parse the payload as an ARPMessage and, if successful,
		// remember the mapping between the sender’s IP address and Ethernet address for
		// 30 seconds. (Learn mappings from both requests and replies.)
		//If it’s an ARP request asking for our IP address, send an appropriate ARP reply
        ARPMessage arp_msg;
        if (arp_msg.parse(frame.payload()) == ParseResult::NoError) {
            // Learn the mapping from the sender's IP address to their Ethernet address.
            _ip_to_ethernet[arp_msg.sender_ip_address] = arp_msg.sender_ethernet_address;
			// Reset the stored time for this ARP entry to 0, so it will expire after 30 seconds.
			_arp_stored_time[arp_msg.sender_ip_address] = 0;
			_pending_arp_requests.erase(arp_msg.sender_ip_address);
            _pending_arp_request_time.erase(arp_msg.sender_ip_address);

            // If there are any datagrams waiting for this IP address, send them now.
            auto waiting_datagrams = _waiting_datagrams.find(arp_msg.sender_ip_address);
            if (waiting_datagrams != _waiting_datagrams.end()) {
                while (!waiting_datagrams->second.empty()) {
                    InternetDatagram dgram = waiting_datagrams->second.front();
                    waiting_datagrams->second.pop();
                    send_datagram(dgram, Address::from_ipv4_numeric(arp_msg.sender_ip_address));
                }
            }

            if (arp_msg.opcode == ARPMessage::OPCODE_REQUEST && arp_msg.target_ip_address == _ip_address.ipv4_numeric()) {
                // This is an ARP request for our IP address, so we need to send an ARP reply.

                ARPMessage reply_msg;
                reply_msg.opcode = ARPMessage::OPCODE_REPLY;
                reply_msg.sender_ethernet_address = _ethernet_address;
                reply_msg.sender_ip_address = _ip_address.ipv4_numeric();
                reply_msg.target_ethernet_address = arp_msg.sender_ethernet_address;
                reply_msg.target_ip_address = arp_msg.sender_ip_address;

                EthernetFrame reply_frame;
                reply_frame.header().type = EthernetHeader::TYPE_ARP;
                reply_frame.header().src = _ethernet_address;
                reply_frame.header().dst = arp_msg.sender_ethernet_address;
                reply_frame.payload() = BufferList(reply_msg.serialize());
                _frames_out.push(reply_frame);
            }
        }
    }

    return nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
	for (auto it = _arp_stored_time.begin(); it != _arp_stored_time.end();) {
        it->second += ms_since_last_tick;
        if (it->second >= 30000) {
            // This ARP entry has been stored for 30 seconds or more, so remove it.
            uint32_t ip = it->first;
            it = _arp_stored_time.erase(it);
            _ip_to_ethernet.erase(ip);
        } else {
            ++it;
        }
    }

	for (auto it = _pending_arp_request_time.begin(); it != _pending_arp_request_time.end();) {
        it->second += ms_since_last_tick;
        if (it->second >= 5000) {
            // This ARP request has been pending for 5 seconds or more, so remove it.
            uint32_t ip = it->first;
            it = _pending_arp_request_time.erase(it);
            _pending_arp_requests.erase(ip);
            _waiting_datagrams.erase(ip);
        } else {
            ++it;
        }
    }
}
