#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <queue>

class RetransmissionTimer
{
public:
  RetransmissionTimer( uint64_t RTO_ms ) : RTO_ms_( RTO_ms ) {}
  bool is_actived() const noexcept { return actived_; }
  bool is_expired() const noexcept { return actived_ && time_elapsed >= RTO_ms_; }
  void stop() noexcept { actived_ = false; }

  void active() noexcept;
  void tick( uint64_t ms_since_last_tick ) noexcept;
  void reset() noexcept;
  void expand() noexcept;
  void set_RTO( uint64_t ms ) noexcept;

private:
  uint64_t RTO_ms_;
  uint64_t time_elapsed { 0 };
  bool actived_ {};
};

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  std::queue<TCPSenderMessage> outstanding_messages_ {};

  bool SYN_flag_ {};
  bool FIN_flag_ {};

  uint64_t wnd_size_ { 1 };
  uint64_t sentno_ { 0 };
  uint64_t ackno_ { 0 };

  uint64_t retransmission_times_ { 0 };

  RetransmissionTimer timer_ { initial_RTO_ms_ };
};
