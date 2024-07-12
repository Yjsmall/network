#include "byte_stream.hh"
#include <cstdio>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return closed_;
}

void Writer::push( string data )
{
  if ( is_closed() ) {
    return;
  }

  if ( data.size() > available_capacity() ) {
    data.resize( available_capacity() );
  }

  if ( !data.empty() ) {
    write_bytes_size_ += data.size();
    buffer_size_ += data.size();
    buffer_.emplace_back( data );
  }

  if ( wnd.empty() && !buffer_.empty() ) {
    wnd = buffer_.front();
  }
}

void Writer::close()
{
  if ( !is_closed() ) {
    closed_ = true;
    buffer_.emplace_back( std::string( 1, EOF ) );
  }
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_size_;
}

uint64_t Writer::bytes_pushed() const
{
  return write_bytes_size_;
}

/* Reader implementation */

bool Reader::is_finished() const
{
  return closed_ && bytes_buffered() == 0;
}

uint64_t Reader::bytes_popped() const
{
  return read_bytes_size_;
}

string_view Reader::peek() const
{
  return wnd;
}

void Reader::pop( uint64_t len )
{
  auto remainder = len;
  while ( remainder >= wnd.size() && remainder != 0 ) {
    remainder -= wnd.size();
    buffer_.pop_front();
    wnd = buffer_.empty() ? string_view() : buffer_.front();
  }

  if ( !wnd.empty() ) {
    wnd.remove_prefix( remainder );
  }

  read_bytes_size_ += len;
  buffer_size_ -= len;
}

uint64_t Reader::bytes_buffered() const
{
  return buffer_size_;
}
