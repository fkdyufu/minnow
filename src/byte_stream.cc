#include "byte_stream.hh"
#include <string_view>
using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return closed_;
}

void Writer::push( string data )
{
  uint64_t available_capacity = this->available_capacity();
  uint64_t bytes_to_write = std::min( (uint64_t)data.size(), available_capacity );
  buffer_.append( data, 0, bytes_to_write );
  bytes_pushed_ += bytes_to_write;
}

void Writer::close()
{
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_.size();
}

uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_;
}

bool Reader::is_finished() const
{
  return closed_ && buffer_.size() == 0;
}

uint64_t Reader::bytes_popped() const
{
  return bytes_popped_;
}

string_view Reader::peek() const
{
  return buffer_;
}

void Reader::pop( uint64_t len )
{
  uint64_t bytes_pop = std::min( len, buffer_.size() );
  if ( !buffer_.empty() ) {
    buffer_.erase( 0, bytes_pop );
  }
  bytes_popped_ += bytes_pop;
  return;
}

uint64_t Reader::bytes_buffered() const
{
  return buffer_.size();
}
