#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  _trie.insert(RouteEntry(route_prefix, prefix_length, next_hop, interface_num));
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{ 
  queue<InternetDatagram> test;
  for( auto& interface : _interfaces ) {
    while (!interface->datagrams_received().empty()) {
      InternetDatagram& x = interface->datagrams_received().front();
      match_and_route(x);
      interface->datagrams_received().pop();
    }
  }
}

size_t Router::match_and_route(InternetDatagram& datagram) {
  if (datagram.header.ttl<= 1) {
    return -1;
  }

  uint32_t ip = datagram.header.dst;
  auto match_result_ = _trie.match(ip);
  if (!match_result_.has_value()) {
    return -1;
  }
  RouteEntry match_result = match_result_.value();
  
  datagram.header.ttl--;
  datagram.header.compute_checksum();
  interface(match_result.interface_num)->send_datagram( datagram, match_result.next_hop.has_value()?match_result.next_hop.value():Address::from_ipv4_numeric(ip));
  return 0;
}