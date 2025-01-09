#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

static ARPMessage make_arp( const uint16_t opcode,
                            const EthernetAddress sender_ethernet_address,
                            const uint32_t sender_ip_address,
                            const EthernetAddress target_ethernet_address,
                            const uint32_t target_ip_address );

static EthernetFrame make_frame( const EthernetAddress& src,
                                 const EthernetAddress& dst,
                                 const uint16_t type,
                                 vector<string> payload );

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  auto it = arp_table_.find( next_hop.ipv4_numeric() );
  if ( it != arp_table_.end() ) {
    transmit( make_frame( ethernet_address_, it->second.first, EthernetHeader::TYPE_IPv4, serialize( dgram ) ) );
  } else {
    datagrams_wait_send_.push_back( { dgram, next_hop } );
    if ( arp_sended_.find( next_hop.ipv4_numeric() ) == arp_sended_.end() ) {
      transmit( make_frame( ethernet_address_,
                            ETHERNET_BROADCAST,
                            EthernetHeader::TYPE_ARP,
                            serialize( make_arp( ARPMessage::OPCODE_REQUEST,
                                                 ethernet_address_,
                                                 ip_address_.ipv4_numeric(),
                                                 { 0 },
                                                 next_hop.ipv4_numeric() ) ) ) );
      arp_sended_.insert( { next_hop.ipv4_numeric(), current_time_ms_ } );
    }
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if ( frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_ ) {
    return;
  }

  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram ip;
    if ( parse( ip, frame.payload ) ) {
      datagrams_received_.push( ip );
    }
  } else if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arp;
    if ( parse( arp, frame.payload ) ) {
      arp_table_.insert( { arp.sender_ip_address, { arp.sender_ethernet_address, current_time_ms_ } } );
    } else {
      std::cerr << "parse arp failed!" << std::endl;
      return;
    }
    if ( arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == ip_address_.ipv4_numeric() ) {
      transmit( make_frame( ethernet_address_,
                            arp.sender_ethernet_address,
                            EthernetHeader::TYPE_ARP,
                            serialize( make_arp( ARPMessage::OPCODE_REPLY,
                                                 ethernet_address_,
                                                 ip_address_.ipv4_numeric(),
                                                 arp.sender_ethernet_address,
                                                 arp.sender_ip_address ) ) ) );
    } else if ( arp.opcode == ARPMessage::OPCODE_REPLY ) {
      // resend ipv4 frame when arp mapping update
      auto it = datagrams_wait_send_.begin();
      while ( it != datagrams_wait_send_.end() ) {
        if ( it->second.ipv4_numeric() == arp.sender_ip_address ) {
          send_datagram( it->first, it->second );
          it = datagrams_wait_send_.erase( it );
        } else {
          ++it;
        }
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  current_time_ms_ += ms_since_last_tick;
  {
    auto it = arp_table_.begin();
    while ( it != arp_table_.end() ) {
      if ( it->second.second + ms_address_mapping_timeout_ <= current_time_ms_ ) {
        it = arp_table_.erase( it );
      } else {
        ++it;
      }
    }
  }
  {
    auto it = arp_sended_.begin();
    while ( it != arp_sended_.end() ) {
      if ( it->second + ms_arp_send_interval_ <= current_time_ms_ ) {
        it = arp_sended_.erase( it );
      } else {
        ++it;
      }
    }
  }
}

static ARPMessage make_arp( const uint16_t opcode,
                            const EthernetAddress sender_ethernet_address,
                            const uint32_t sender_ip_address,
                            const EthernetAddress target_ethernet_address,
                            const uint32_t target_ip_address )
{
  ARPMessage arp;
  arp.opcode = opcode;
  arp.sender_ethernet_address = sender_ethernet_address;
  arp.sender_ip_address = sender_ip_address;
  arp.target_ethernet_address = target_ethernet_address;
  arp.target_ip_address = target_ip_address;
  return arp;
}

static EthernetFrame make_frame( const EthernetAddress& src,
                                 const EthernetAddress& dst,
                                 const uint16_t type,
                                 vector<string> payload )
{
  EthernetFrame frame;
  frame.header.src = src;
  frame.header.dst = dst;
  frame.header.type = type;
  frame.payload = std::move( payload );
  return frame;
}