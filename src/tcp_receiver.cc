#include "tcp_receiver.hh"
#include<iostream>
using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if(message.RST){
    reassembler_.reader().set_error();
    return;
  }

  uint64_t checkpoint = reassembler_.writer().bytes_pushed() + ISN.has_value();
  
  if (ISN.has_value() && message.seqno == ISN ){
    return;
  }
  if (!ISN.has_value()) {
    if (!message.SYN) return;
    ISN = message.seqno;
  }
  uint64_t seqno = message.seqno.unwrap(*ISN, checkpoint);
  uint64_t index = seqno == 0? 0: seqno - 1;
  reassembler_.insert(index, move(message.payload), message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  uint64_t ACK = reassembler_.get_head_index() + ISN.has_value();
  uint16_t window_size = min(reassembler_.writer().available_capacity(), 65535ul);
  
  if (!ISN.has_value()){
	return TCPReceiverMessage({}, window_size, reassembler_.writer().has_error());
  }
  uint64_t FIN = reassembler_.writer().is_closed();
  return TCPReceiverMessage(Wrap32::wrap(ACK + FIN, *ISN), window_size, reassembler_.writer().has_error());
}
