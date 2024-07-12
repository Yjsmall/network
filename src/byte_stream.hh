#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <string_view>

class Reader;
class Writer;

class ByteStream
{
public:
  explicit ByteStream( uint64_t capacity );

  // Helper functions (provided) to access the ByteStream's Reader and Writer interfaces
  Reader& reader();
  const Reader& reader() const;
  Writer& writer();
  const Writer& writer() const;

  void set_error() { error_ = true; };       // Signal that the stream suffered an error.
  bool has_error() const { return error_; }; // Has the stream had an error?

protected:
  // Please add any additional state to the ByteStream here, and not to the Writer and Reader interfaces.
  uint64_t capacity_;
  bool error_ {};
  bool closed_ {};

  std::string_view wnd {};

  std::deque<std::string> buffer_ {};
  uint64_t buffer_size_ {};

  uint64_t write_bytes_size_ {};
  uint64_t read_bytes_size_ {};
};

class Writer : public ByteStream
{
public:
  // Push data to stream, but only as much as available capacity allows.
  void push( std::string data );
  // Signal that the stream has reached its ending. Nothing more will be written.
  void close();
  // Has the stream been closed?
  bool is_closed() const;
  // How many bytes can be pushed to the stream right now?
  uint64_t available_capacity() const;
  // Total number of bytes cumulatively pushed to the stream
  uint64_t bytes_pushed() const;
};

class Reader : public ByteStream
{
public:
  // Peek at the next bytes in the buffer
  std::string_view peek() const;
  // Remove `len` bytes from the buffer
  void pop( uint64_t len );
  // Is the stream finished (closed and fully popped)?
  bool is_finished() const;
  // Number of bytes currently buffered (pushed and not popped)
  uint64_t bytes_buffered() const;
  // Total number of bytes cumulatively popped from stream
  uint64_t bytes_popped() const;
};

/*
 * read: A (provided) helper function thats peeks and pops up to `len` bytes
 * from a ByteStream Reader into a string;
 */
void read( Reader& reader, uint64_t len, std::string& out );
