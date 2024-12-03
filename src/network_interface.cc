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

void NetworkInterface::send_known(){
  for (auto& [key, value] : send_queue_){
    if(arp_cache_.contains(key)){
      while(!value.empty()){
        EthernetFrame frame = value.front();
        frame.header.dst = arp_cache_[key];
        value.pop_front();
        transmit(frame);
      }
    }
  }
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  EthernetFrame data_frame= {
    .header={
      .dst = ETHERNET_BROADCAST,
      .src = ethernet_address_,
      .type = EthernetHeader::TYPE_IPv4,
    },
    .payload= serialize(dgram),
  };
  send_queue_[next_hop.ipv4_numeric()].push_back(data_frame);

  send_known();

  if(!arp_cache_.contains(next_hop.ipv4_numeric())){
    if (arp_time_stamp_.contains(next_hop.ipv4_numeric()) && abs_time_ - arp_time_stamp_[next_hop.ipv4_numeric()] < 5000){
      return ;
    }
    ARPMessage arp_request ={
      .opcode = ARPMessage::OPCODE_REQUEST,
      .sender_ethernet_address = ethernet_address_,
      .sender_ip_address = ip_address_.ipv4_numeric(),
      .target_ethernet_address = 0,
      .target_ip_address = next_hop.ipv4_numeric(),
    };
    EthernetFrame frame = {
      .header={
        .dst = ETHERNET_BROADCAST,
        .src = ethernet_address_,
        .type = EthernetHeader::TYPE_ARP,
      },
      .payload= serialize(arp_request),
    };
    transmit(frame);
    arp_time_stamp_[next_hop.ipv4_numeric()] = abs_time_;
    return ;
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  EthernetHeader header = frame.header;
  if (header.dst != ethernet_address_ && header.dst != ETHERNET_BROADCAST){
    return ;
  }
  if (header.type == EthernetHeader::TYPE_ARP){
    ARPMessage arp_message;
    parse(arp_message, frame.payload);
    arp_cache_[arp_message.sender_ip_address] = arp_message.sender_ethernet_address;
    arp_time_stamp_[arp_message.sender_ip_address] = abs_time_;
    if (arp_message.opcode == ARPMessage::OPCODE_REQUEST && arp_message.target_ip_address == ip_address_.ipv4_numeric()){
      ARPMessage arp_reply = {
        .opcode = ARPMessage::OPCODE_REPLY,
        .sender_ethernet_address = ethernet_address_,
        .sender_ip_address = ip_address_.ipv4_numeric(),
        .target_ethernet_address = arp_message.sender_ethernet_address,
        .target_ip_address = arp_message.sender_ip_address,
      };
      EthernetFrame req_frame = {
        .header={
          .dst = arp_message.sender_ethernet_address,
          .src = ethernet_address_,
          .type = EthernetHeader::TYPE_ARP,
        },
        .payload= serialize(arp_reply),
      };
      transmit(req_frame);
    }
  }else if (header.type == EthernetHeader::TYPE_IPv4){
    InternetDatagram dgram;
    parse(dgram, frame.payload);
    datagrams_received_.push(dgram);
  }
  send_known();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  abs_time_ += ms_since_last_tick;
  for (auto it = arp_time_stamp_.cbegin(); it != arp_time_stamp_.cend(); ){
    if (abs_time_ - it->second > 30000){
      arp_cache_.erase(it->first);
      arp_time_stamp_.erase(it++);
    }else{
      ++it;
    }
  }
}
