#include "tcp_receiver.hh"

using namespace std;

// 接收消息并发送消息
void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( reassembler_.reader().has_error() )
    return;
  if ( message.RST ) {
    reassembler_.reader().set_error();
    return;
  }
  if ( !zero_point_.has_value() && !message.SYN ) {
    // reassembler_.reader().set_error();
    return;
  }
  if ( message.SYN ) {
    zero_point_ = message.seqno;
    message.seqno = message.seqno + 1;
  }
  reassembler_.insert( message.seqno.unwrap( zero_point_.value(), reassembler_.writer().bytes_pushed() ) - 1,
                       message.payload,
                       message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage message { nullopt, 0, false };
  if ( zero_point_.has_value() ) {
    message.ackno = Wrap32::wrap( reassembler_.writer().bytes_pushed() + 1 + reassembler_.writer().is_closed(),
                                  zero_point_.value() );
  }
  message.window_size = std::min( reassembler_.writer().available_capacity(), static_cast<uint64_t>( UINT16_MAX ) );
  message.RST = reassembler_.writer().has_error();
  return message;
}
