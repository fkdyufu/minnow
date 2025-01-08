#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

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
  // Your code here.
  auto it = arp_table_.find( next_hop.ipv4_numeric() );
  if ( it != arp_table_.end() ) {
    EthernetFrame ipv4_frame;
    ipv4_frame.header.type = EthernetHeader::TYPE_IPv4;
    ipv4_frame.header.dst = it->second.first;
    ipv4_frame.header.src = ethernet_address_;
    ipv4_frame.payload = serialize( dgram );
    transmit( ipv4_frame );
  } else {
    if ( arp_sended_.find( next_hop.ipv4_numeric() ) == arp_sended_.end() ) {
      EthernetFrame arp_frame;
      ARPMessage msg;

      msg.opcode = ARPMessage::OPCODE_REQUEST;
      msg.sender_ethernet_address = ethernet_address_;
      msg.sender_ip_address = ip_address_.ipv4_numeric();
      msg.target_ip_address = next_hop.ipv4_numeric();

      arp_frame.header.type = EthernetHeader::TYPE_ARP;
      arp_frame.header.dst = ETHERNET_BROADCAST;
      arp_frame.header.src = ethernet_address_;
      arp_frame.payload = serialize( msg );

      transmit( arp_frame );
      arp_sended_.insert( { next_hop.ipv4_numeric(), current_time_ms_ } );
    }
    datagrams_wait_send_.push_back( { dgram, next_hop } );
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
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
      ARPMessage msg;
      EthernetFrame frame_tmp;
      msg.opcode = ARPMessage::OPCODE_REPLY;
      msg.sender_ip_address = ip_address_.ipv4_numeric();
      msg.target_ip_address = arp.sender_ip_address;
      msg.sender_ethernet_address = ethernet_address_;
      msg.target_ethernet_address = arp.sender_ethernet_address;
      frame_tmp.header.type = EthernetHeader::TYPE_ARP;
      frame_tmp.header.dst = arp.sender_ethernet_address;
      frame_tmp.header.src = ethernet_address_;
      frame_tmp.payload = serialize( msg );
      transmit( frame_tmp );
    } else if ( arp.opcode == ARPMessage::OPCODE_REPLY ) {
      // 清除发送记录
      // 再次发送
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
