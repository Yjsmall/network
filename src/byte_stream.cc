#include "byte_stream.hh"
#include <cstdio>
#include <iterator>
#include <ranges>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), buffer_( capacity_ + 1 ) {}

bool Writer::is_closed() const
{
  return has_close();
}

void Writer::push( string data )
{
  if ( has_close() ) {
    return;
  }

  auto left_size = capacity_ - write_bytes_size_;

  if ( left_size == 0 ) {
    buffer_.push_back( EOF );
    set_close();
  }

  if ( left_size <= data.size() ) {
    std::ranges::copy_n( buffer_.begin(), left_size, std::back_inserter( data ) );
    buffer_.push_back( EOF );
    write_bytes_size_ += left_size;
    set_close();
  }

  std::ranges::copy_n( buffer_.begin(), data.size(), std::back_inserter( data ) );
  write_bytes_size_ += data.size();
}

void Writer::close()
{
  // Your code here.
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return {};
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return {};
}

bool Reader::is_finished() const
{
  // Your code here.
  return {};
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return {};
}

string_view Reader::peek() const
{
  // Your code here.
  return {};
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  (void)len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return {};
}
