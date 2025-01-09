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

  routes_.push_back( { route_prefix, prefix_length, next_hop, interface_num } );
}

std::optional<Router::Route_> Router::match_route( uint32_t address )
{
  std::optional<Route_> best_match;
  uint8_t max_prefix_length = 0;
  uint32_t mask = 0xffffffff;
  for ( const auto& route : routes_ ) {
    if ( route.prefix_length != 32 ) {
      mask >>= route.prefix_length;
      mask <<= route.prefix_length;
    }
    if ( ( max_prefix_length == 0 || route.prefix_length > max_prefix_length )
         && ( address & mask ) == ( route.route_prefix & mask ) ) {
      best_match = route;
      max_prefix_length = route.prefix_length;
    }
  }

  return best_match;
}
// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for ( const auto& net_iface : _interfaces ) {
    std::queue<InternetDatagram>& datagrames = net_iface->datagrams_received();
    while ( !datagrames.empty() ) {
      InternetDatagram inet_dgram = datagrames.front();
      datagrames.pop();
      // inet_dgram.header.dst;
      std::optional<Route_> route = match_route( inet_dgram.header.dst );
      if ( !route.has_value() || inet_dgram.header.ttl == 0 || --inet_dgram.header.ttl == 0 ) {
        continue;
      }
      _interfaces[route.value().interface_num]->send_datagram(
        inet_dgram, route.value().next_hop.value_or( Address::from_ipv4_numeric( 0 ) ) );
    }
  }
}
