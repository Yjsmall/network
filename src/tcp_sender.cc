#include "tcp_sender.hh"
#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"
#include <algorithm>
#include <string>
#include <string_view>

using namespace std;

// Returns the number of sequence numbers that have been sent but not yet acknowledged
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return sentno_ - ackno_;
}

// Returns the number of consecutive retransmissions that have occurred
uint64_t TCPSender::consecutive_retransmissions() const
{
  return retransmission_times_;
}

// Pushes data into the TCP sender's output buffer and manages retransmission logic
void TCPSender::push( const TransmitFunction& transmit )
{
  // Continue sending data as long as there's space in the window
  while ( ( wnd_size_ == 0 ? 1 : wnd_size_ ) > sequence_numbers_in_flight() ) {
    if ( FIN_flag_ ) {
      break; // Stop if FIN flag is already set
    }

    // Create a new TCP message with empty payload
    auto msg = make_empty_message();

    if ( !SYN_flag_ ) {
      // Set SYN flag on the first message
      msg.SYN = true;
      SYN_flag_ = true;
    }

    // Calculate the remaining space in the window
    uint64_t remaining_wnd_space = ( wnd_size_ == 0 ? 1 : wnd_size_ ) - sequence_numbers_in_flight();
    uint64_t len = min( TCPConfig::MAX_PAYLOAD_SIZE, remaining_wnd_space - msg.sequence_length() );

    auto& datas = msg.payload;

    // NOTE: here must use reference not directly use reader_end, it will copy it.
    auto& reader_end = input_.reader();
    // Fill the message payload with data from the input buffer
    while ( reader_end.bytes_buffered() && datas.size() < len ) {
      auto cur_datas = reader_end.peek();
      cur_datas = cur_datas.substr( 0, len - datas.size() );
      datas += cur_datas;
      reader_end.pop( datas.size() );
    }

    if ( !FIN_flag_ && remaining_wnd_space > msg.sequence_length() && reader_end.is_finished() ) {
      // Set FIN flag if all data has been read and there's space left in the window
      msg.FIN = true;
      FIN_flag_ = true;
    }

    if ( msg.sequence_length() == 0 ) {
      break; // Stop if the message has no data to send
    }
    transmit( msg ); // Send the message
    if ( !timer_.is_actived() ) {
      // Start and reset the retransmission timer if not already active
      timer_.active();
      timer_.reset();
    }

    sentno_ += msg.sequence_length();  // Update the sequence number
    outstanding_messages_.push( msg ); // Store the message in the queue for retransmission
  }
}

// Creates an empty TCP message with initial settings
TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage msg { Wrap32::wrap( sentno_, isn_ ), false, "", false, input_.has_error() };
  return msg;
}

// Handles the receipt of an acknowledgment or window update from the receiver
void TCPSender::receive( const TCPReceiverMessage& msg )
{
  wnd_size_ = msg.window_size; // Update the window size

  if ( msg.RST ) {
    // If RST (reset) flag is set, signal an error and stop processing
    input_.reader().set_error();
    return;
  }

  if ( msg.ackno.has_value() ) {
    // If an acknowledgment number is present, process it
    const uint64_t ackno = msg.ackno.value().unwrap( isn_, ackno_ );
    if ( ackno > sentno_ ) {
      return; // Ignore invalid acknowledgments
    }

    while ( !outstanding_messages_.empty() ) {
      auto first = outstanding_messages_.front();
      if ( ackno < ackno_ + first.sequence_length() ) {
        break; // Stop if the acknowledgment does not cover the entire message
      }

      // Acknowledge the message and remove it from the queue
      ackno_ += first.sequence_length();
      outstanding_messages_.pop();
      retransmission_times_ = 0;         // Reset the retransmission counter
      timer_.set_RTO( initial_RTO_ms_ ); // Reset the retransmission timeout
      timer_.reset();                    // Reset the timer
      if ( outstanding_messages_.empty() ) {
        timer_.stop(); // Stop the timer if there are no outstanding messages
      }
    }
  }
}

// Handles the passage of time and retransmission logic
void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if ( timer_.is_actived() ) {
    timer_.tick( ms_since_last_tick ); // Update the timer with the elapsed time
  }

  if ( timer_.is_expired() ) {
    // If the timer has expired, retransmit the first unacknowledged message
    while ( !outstanding_messages_.empty() ) {
      auto msg = outstanding_messages_.front();
      auto idx = msg.seqno.unwrap( isn_, sentno_ );

      if ( idx + msg.sequence_length() > ackno_ ) {
        transmit( msg ); // Retransmit the message

        if ( wnd_size_ ) {
          retransmission_times_++; // Increment the retransmission counter
          timer_.expand();         // Double the retransmission timeout
        }

        timer_.reset(); // Reset the timer after retransmission
        break;

      } else {
        outstanding_messages_.pop(); // Remove fully acknowledged messages from the queue
      }
    }
  }
}

// Implementation of the RetransmissionTimer class

void RetransmissionTimer::active() noexcept
{
  actived_ = true; // Activate the retransmission timer
}

void RetransmissionTimer::expand() noexcept
{
  RTO_ms_ *= 2; // Double the retransmission timeout value
}

void RetransmissionTimer::reset() noexcept
{
  time_elapsed = 0; // Reset the elapsed time to zero
}

void RetransmissionTimer::tick( uint64_t ms_since_last_tick ) noexcept
{
  if ( actived_ ) {
    time_elapsed += ms_since_last_tick; // Update the elapsed time
  }
}

void RetransmissionTimer::set_RTO( uint64_t ms ) noexcept
{
  RTO_ms_ = ms; // Set a new retransmission timeout value
}
