#include "reassembler.hh"
#include <vector>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_prev_substring )
{
  auto& writer = output_.writer();

  // Sliding Window: all elems are inserted in this interval
  uint64_t wnd_start = nxt_assembled_idx_;
  uint64_t wnd_end = wnd_start + writer.available_capacity();
  // Interval where the element was inserted
  uint64_t cur_start = first_index;
  uint64_t cur_end = cur_start + data.size();

  if ( is_prev_substring ) {
    EOF_idx_ = cur_end;
  }

  if ( cur_start >= wnd_end ) {
    return;
  }

  cur_start = max( wnd_start, cur_start );
  cur_end = min( wnd_end, cur_end );
  if ( cur_start >= cur_end ) {
    if ( nxt_assembled_idx_ == EOF_idx_ ) {
      writer.close();
    }
    return;
  }

  uint64_t len = cur_end - cur_start;

  buffer_.insert( Interval { cur_start, cur_end, data.substr( cur_start - first_index, len ) } );

  merge_intervel();

  auto it = buffer_.begin();
  while ( it->start_ == nxt_assembled_idx_ ) {
    writer.push( it->data_ );
    nxt_assembled_idx_ = it->end_;
    it = buffer_.erase( it );
  }

  if ( nxt_assembled_idx_ == EOF_idx_ ) {
    writer.close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  uint64_t pending_size { 0 };
  for ( const auto& interval : buffer_ ) {
    pending_size += interval.end_ - interval.start_;
  }

  return pending_size;
}

void Reassembler::merge_intervel()
{
  std::vector<Interval> merged;
  auto it = buffer_.begin();
  Interval prev = *it;
  it++;

  while ( it != buffer_.end() ) {
    if ( it->start_ <= prev.end_ ) {
      if ( prev.end_ < it->end_ ) {
        prev.end_ = it->end_;
        prev.data_ = prev.data_.substr( 0, it->start_ - prev.start_ ) + it->data_;
      }
    } else {
      merged.push_back( prev );
      prev = *it;
    }
    it++;
  }

  merged.push_back( prev );
  buffer_.clear();
  for ( const auto& interval : merged ) {
    buffer_.insert( interval );
  }
}