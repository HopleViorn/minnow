#include "tcp_sender.hh"
#include "tcp_config.hh"
using namespace std;
#include<iostream>
#define zxc(x) cerr<<(#x)<<'='<<(x)<<'\n'

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return bytes_in_flight;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
	return retx_cnt;
}

void TCPSender::push( const TransmitFunction& transmit )
{
	Reader &reader = input_.reader();

	uint64_t pseudo_cwnd = cwnd == 0? 1: cwnd;
	uint64_t max_size =  pseudo_cwnd < sequence_numbers_in_flight() ? 0: pseudo_cwnd - sequence_numbers_in_flight();
	
	bool fin = reader.is_finished() && !finished;
	if (reader.bytes_buffered() == 0){
		TCPSenderMessage msg = make_empty_message();
		if(!syn_sent){
			msg.SYN = true;
			bytes_in_flight++;
			next_seqno = next_seqno + 1;
			syn_sent = true;
		}
		if(max_size >=1 && fin){
			msg.FIN = true;
			bytes_in_flight++;
			next_seqno = next_seqno + 1;
			finished = true;
		}
		if(msg.FIN||msg.SYN){
			transmit(msg);
			buffered_messages.emplace_back(msg);
			timer_on = true;
		}
		return ;
	}
	uint64_t bytes_sent = 0;
	while(bytes_sent < max_size){
		string_view head = reader.peek();
		if(head.size() == 0){
			break;
		}
		TCPSenderMessage msg = make_empty_message();

		if(!syn_sent){
			msg.SYN = true;
			syn_sent = true;
			bytes_in_flight++;
			next_seqno = next_seqno + 1;
			bytes_sent++;
		}
		uint64_t delta = min(min(head.size(), max_size - bytes_sent),TCPConfig::MAX_PAYLOAD_SIZE);
		if(delta == 0) break;
		string payload = string(head.substr(0, delta));
		reader.pop(delta);
		fin = reader.is_finished();

		msg.payload = move(payload);
		bytes_sent += delta;

		if(fin && max_size - bytes_sent >= 1){
			msg.FIN = true;
			bytes_in_flight++;
			next_seqno = next_seqno + 1;
			finished = true;
		}
		transmit(msg);
		buffered_messages.emplace_back(msg);
		bytes_in_flight+=delta;
		next_seqno = next_seqno + delta;
		timer_on = true;
	}
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  return {
    .seqno = Wrap32::wrap(next_seqno, isn_),
    .SYN = false,
    .payload = {},
    .FIN = false,
    .RST = input_.reader().has_error()
  };
}

void TCPSender::receive( const TCPReceiverMessage& msg ){
	cwnd = msg.window_size;
	if ( msg.RST ) {
		input_.set_error();
		return ;
	}
	if(msg.ackno.has_value()){
		uint64_t ack = msg.ackno.value().unwrap(isn_, next_seqno);
		if(ack > next_seqno) return ;
		while(!buffered_messages.empty() && ack >= buffered_messages.front().seqno.unwrap(isn_, next_seqno)+buffered_messages.front().payload.size()+buffered_messages.front().FIN + buffered_messages.front().SYN){
			bytes_in_flight -= buffered_messages.front().payload.size();
			bytes_in_flight -= buffered_messages.front().FIN;
			bytes_in_flight -= buffered_messages.front().SYN;
			buffered_messages.pop_front();
			timer_ms = 0;
			retx_cnt = 0;
			rto_multiplier = 1;
			if(buffered_messages.empty()){
				timer_on = false;
			}
		}
	}
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
	if (!timer_on){
		return ;
	}
	timer_ms += ms_since_last_tick;
	if(timer_ms >= initial_RTO_ms_ * rto_multiplier && !buffered_messages.empty()){
		TCPSenderMessage msg = buffered_messages.front();
		retx_cnt++;
		transmit(msg);
		if(cwnd) rto_multiplier *= 2;
		timer_ms = 0;
	}
}
