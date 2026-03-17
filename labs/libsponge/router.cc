#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    /*
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";
    */
    // Your code here.
    _routing_table.push_back({route_prefix, prefix_length, next_hop, interface_num});
     // Sort the routing table so that entries with longer prefix lengths appear first
    sort(_routing_table.begin(), _routing_table.end(), [](const Route &a, const Route &b) {
        return a.prefix_length > b.prefix_length;
    });
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    // Your code here.
    uint32_t dest_ip = dgram.header().dst;
    uint8_t ttl = dgram.header().ttl;
    if (ttl <= 1) {
        // TTL expired, drop the datagram
        return;
    }
    dgram.header().ttl -= 1; // Decrement TTL before forwarding

    // note the route table is sorted by prefix length,
    // so the first match will be the longest prefix match
    for (const auto &route : _routing_table) {
        // the mask is used to compare only the most significant bits of the destination IP and the route prefix
        uint32_t mask = route.prefix_length == 0 ? 0 : (0xFFFFFFFF << (32 - route.prefix_length));
        if ((dest_ip & mask) == (route.route_prefix & mask)) {
            // If next_hop is empty, the datagram is destined for a directly attached network,
            // so use the destination IP as the next hop
            Address next_hop = route.next_hop.has_value() ? route.next_hop.value() : Address::from_ipv4_numeric(dest_ip);
            interface(route.interface_num).send_datagram(dgram, next_hop);
            return;
        }
    }

}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
