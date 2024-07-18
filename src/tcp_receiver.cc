#include "tcp_receiver.hh"
#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "wrapping_integers.hh"
#include <cstdint>
#include <optional>
#include <utility>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  auto const& writer_end = writer();
  if ( writer_end.has_error() ) {
    return;
  }

  if ( message.RST ) {
    reader().set_error();
    return;
  }

  if ( !ISN_.has_value() ) {
    if ( !message.SYN ) {
      return;
    }
    ISN_.emplace( message.seqno );
  }

  Wrap32 zero_point = ISN_.value();
  uint64_t checkpoint = writer_end.bytes_pushed() + static_cast<uint32_t>( message.SYN );
  uint64_t asb_seqno = message.seqno.unwrap( zero_point, checkpoint );
  uint64_t stream_idx = asb_seqno + static_cast<uint64_t>( message.SYN ) - 1;
  reassembler_.insert( stream_idx, std::move( message.payload ), message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  auto const& writer_end = writer();
  uint16_t wnd_size
    = static_cast<uint16_t>( min( writer_end.available_capacity(), static_cast<uint64_t>( UINT16_MAX ) ) );
  bool reset = writer_end.has_error();
  if ( ISN_.has_value() ) {
    Wrap32 ackno
      = Wrap32::wrap( writer_end.bytes_pushed() + static_cast<uint64_t>( writer_end.is_closed() ), ISN_.value() )
        + 1;
    return TCPReceiverMessage { ackno, wnd_size, reset };
  }
  return TCPReceiverMessage { nullopt, wnd_size, reset };
}
