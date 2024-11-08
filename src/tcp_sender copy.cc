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
	if(finished) return ;
	if(!established){
		zxc("---------------------------");
		TCPSenderMessage msg = {
			.seqno = Wrap32::wrap(next_seqno, isn_),
			.SYN = true,
			.payload = {},
			.FIN = false,
			.RST = reader.has_error()
		};
		if(reader.is_finished()){
			msg.FIN = true;
			bytes_in_flight++;
			next_seqno = next_seqno + 1;
			finished = true;
		}
		bytes_in_flight++;
		next_seqno = next_seqno + 1;
		transmit(msg);
		established = true;
    	timer_on = true;
		return ;
	}
	// cwnd = max(cwnd, 1ull);

	uint64_t cwnd_ = cwnd==0? 1: cwnd;

	if(send_fin){
		zxc("before fin");
		zxc(cwnd_);
		zxc(bytes_in_flight);
		if(bytes_in_flight +1 > cwnd_) return ;
		TCPSenderMessage msg = {
			.seqno = Wrap32::wrap(next_seqno, isn_),
			.SYN = false,
			.payload = {},
			.FIN = true,
			.RST = reader.has_error()
		};
		buffered_messages.emplace_back(msg);
		transmit(msg);
		bytes_in_flight++;
		next_seqno = next_seqno + 1;
		timer_on = true;
		finished = true;
		return ;
	}

	uint64_t max_size = min(cwnd_ - bytes_in_flight, TCPConfig::MAX_PAYLOAD_SIZE);
	bool keep_on = false;
	while(true){
		string ready_send = {};
		if(finished) break;
		if(bytes_in_flight >= cwnd_) break;

		while(true){
			string_view head = reader.peek();
			if(head.size() == 0){
				break;
			}
			uint64_t delta = min(ready_send.size() + head.size(), max_size) - ready_send.size();
			if(delta <=0 ){
				keep_on = true;
				break;
			}

			ready_send.append(head.substr(0, delta));
			reader.pop(delta);
		}

		if(reader.is_finished()){
			send_fin = true;
		}

		if(ready_send.size() > 0){
			zxc(ready_send);
			uint64_t size = ready_send.size();
			TCPSenderMessage msg = {
				.seqno = Wrap32::wrap(next_seqno, isn_),
				.SYN = false,
				.payload = move(ready_send),
				.FIN = false,
				.RST = reader.has_error()
			};
			if(send_fin&& size + 1 <= cwnd_ - bytes_in_flight){
				zxc("FIN");
				msg.FIN = true;
				next_seqno ++;
				bytes_in_flight ++;
				finished=true;
			}
			buffered_messages.emplace_back(msg);
			transmit(msg);
			next_seqno = next_seqno + size;
			bytes_in_flight += size;
			timer_on = true;
		}else{
			if(send_fin&& cwnd_ - bytes_in_flight > 0){
				zxc("!");
				zxc(next_seqno);
				TCPSenderMessage msg = {
					.seqno = Wrap32::wrap(next_seqno, isn_),
					.SYN = false,
					.payload = {},
					.FIN = true,
					.RST = reader.has_error()
				};
				buffered_messages.emplace_back(msg);
				transmit(msg);
				bytes_in_flight++;
				next_seqno = next_seqno + 1;
				timer_on = true;
				finished = true;
			}
		}
		if(!keep_on) break;
	}
}

//[SYN] [         ] [  ] [    ] [   ]

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
	zxc("receive");
    if(msg.ackno.has_value()){
		uint64_t ack = msg.ackno.value().unwrap(isn_, next_seqno);
		zxc(ack); 
		zxc(next_seqno);
		// if(ack >= next_seqno) return ;
		cwnd = msg.window_size;
		if(!syn_acked && ack == 1){//SYN
			cwnd = msg.window_size;
			bytes_in_flight --;
			timer_ms = 0;
			retx_cnt = 0;
			rto_multiplier = 1;
			syn_acked = true;
			zxc("SYNC");
			timer_on = false;
			return ;
		}
		bool success = false;
		while(!buffered_messages.empty() && ack > buffered_messages.front().seqno.unwrap(isn_, next_seqno) && ack >= buffered_messages.front().seqno.unwrap(isn_, next_seqno)+buffered_messages.front().payload.size()+buffered_messages.front().FIN){
			zxc(buffered_messages.front().seqno.unwrap(isn_, next_seqno));
			zxc(buffered_messages.front().payload);
			bytes_in_flight -= buffered_messages.front().payload.size();
			bytes_in_flight -= buffered_messages.front().FIN;
			buffered_messages.pop_front();
			success = true;
		}
		if(success){
			cwnd = msg.window_size;
      		zxc("success");
			timer_ms = 0;
			retx_cnt = 0;
			rto_multiplier = 1;
			if(buffered_messages.empty()){
				timer_on = false;
			}
		}
	}else{
		if(msg.window_size==0){
			input_.set_error();
		}
	}
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
	zxc(initial_RTO_ms_ * rto_multiplier);
	if (!timer_on){
		return ;
	}
	timer_ms += ms_since_last_tick;
	zxc(timer_ms);
	if(timer_ms >= initial_RTO_ms_ * rto_multiplier){
		if(next_seqno == 1 &&!syn_acked){//retransmit SYN
			TCPSenderMessage msg = {
				.seqno = Wrap32::wrap(0 , isn_),
				.SYN = true,
				.payload = {},
				.FIN = false,
				.RST = false
			};
			transmit(msg);
			// initial_RTO_ms_ = initial_RTO_ms_ * 2;
			zxc(cwnd);
			if(cwnd) rto_multiplier *= 2;
			timer_ms = 0;
			retx_cnt++;
			return ;
		}
		if(buffered_messages.empty()){
			return ;
		}
		TCPSenderMessage msg = buffered_messages.front();
		retx_cnt++;
		transmit(msg);
		zxc(cwnd);
		if(cwnd) rto_multiplier *= 2;
		timer_ms = 0;
	}
}
