#include "tcp_sender.hh"
#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_sender_message.hh"
#include "timer.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  uint64_t first_index = 0, last_index = 0;
  if ( outstanding_segments_queue_.size() != 0 ) {
    last_index = outstanding_segments_queue_.back().seqno.unwrap( isn_, next_seqno_ )
                 + outstanding_segments_queue_.back().sequence_length();
    first_index = max( outstanding_segments_queue_.front().seqno.unwrap( isn_, next_seqno_ ),
                       ackno_.unwrap( isn_, next_seqno_ ) );
  }
  return last_index - first_index;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return retransmissions_count_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  uint16_t width = max( static_cast<uint16_t>( 1 ), width_ );
  if ( ackno_.unwrap( isn_, next_seqno_ ) + width <= next_seqno_ ) { // bytes in window is already send
    return;
  }

  width = ackno_.unwrap( isn_, next_seqno_ ) + width - next_seqno_;
  while ( width > 0 && !flag_FIN_ ) {
    string_view input_string = input_.reader().peek();
    TCPSenderMessage message;
    message.SYN = ( next_seqno_ == 0 );
    message.seqno = Wrap32::wrap( next_seqno_, isn_ );
    // payload_size <= min(width - SYN, MAX_PAYLOAD_SIZE)
    // payload_size <= input_string.size()
    // so. payload_size = min(min(width, MAX_PAYLOAD_SIZE) - SYN, input_string.size());
    size_t payload_len
      = min( min( static_cast<size_t>( width ) - message.SYN, TCPConfig::MAX_PAYLOAD_SIZE ), input_string.size() );
    message.payload = input_string.substr( 0, payload_len );
    input_.reader().pop( payload_len );
    if ( message.sequence_length() < width && input_.reader().is_finished() ) {
      message.FIN = true;
      flag_FIN_ = true;
    }
    if ( message.sequence_length() == 0 ) {
      break;
    }
    message.RST = input_.has_error();
    transmit( message );
    width -= message.sequence_length();
    next_seqno_ += message.sequence_length();
    if ( !retransmit_timer_.is_running() ) {
      retransmit_timer_.start( RTO_ms_ );
    }
    outstanding_segments_queue_.push_back( message );
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return { .seqno = Wrap32::wrap( next_seqno_, isn_ ),
           .SYN = false,
           .payload {},
           .FIN = false,
           .RST = input_.has_error() };
}

// 更新window width 大小
void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( msg.ackno.value_or( isn_ ).unwrap( isn_, next_seqno_ ) > next_seqno_ )
    return;
  if ( msg.ackno.value_or( isn_ ).unwrap( isn_, next_seqno_ ) > ackno_.unwrap( isn_, next_seqno_ ) ) {
    RTO_ms_ = initial_RTO_ms_;
    if ( outstanding_segments_queue_.size() != 0 ) {
      retransmit_timer_.start( RTO_ms_ );
      retransmissions_count_ = 0;
    }
  }
  ackno_ = msg.ackno.value_or( isn_ );
  width_ = msg.window_size;
  if ( msg.RST ) {
    input_.set_error();
  }
  while ( outstanding_segments_queue_.size() != 0 ) {
    TCPSenderMessage outstanding_segment = *outstanding_segments_queue_.begin();
    if ( outstanding_segment.seqno.unwrap( isn_, next_seqno_ ) + outstanding_segment.sequence_length()
         <= ackno_.unwrap( isn_, next_seqno_ ) ) {
      outstanding_segments_queue_.pop_front();
    } else {
      break;
    }
  }
  if ( outstanding_segments_queue_.size() == 0 ) {
    retransmit_timer_.stop();
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  live_time_ms_ += ms_since_last_tick;
  retransmit_timer_.past( ms_since_last_tick );
  if ( retransmit_timer_.is_expired() && outstanding_segments_queue_.size() != 0 ) {
    transmit( *outstanding_segments_queue_.begin() );
    if ( width_ != 0 ) {
      retransmissions_count_++;
      RTO_ms_ <<= 1;
    }
    retransmit_timer_.start( RTO_ms_ );
  }
}
