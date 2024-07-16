#include "reassembler.hh"
#include <vector>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  auto writer = output_.writer();

  // Sliding Window: all elems are inserted in this interval
  uint64_t wnd_start = nxt_assembled_idx_;
  uint64_t wnd_end = wnd_start + writer.available_capacity();
  // Interval where the element was inserted
  uint64_t cur_start = first_index;
  uint64_t cur_end = cur_start + data.size();

  if (is_last_substring) {
    EOF_idx_ = cur_end;
  }

  if (cur_start >= wnd_end) {
    return;
  }

  uint64_t start_idx = max(wnd_start, cur_start);
  uint64_t end_idx = min(wnd_end, cur_end);
  if (start_idx >= end_idx) {
    if (nxt_assembled_idx_ == EOF_idx_) {
      writer.close();
    }
    return;
  }

  uint64_t len = end_idx - start_idx;

  buffer_.insert(Interval{start_idx, end_idx, data.substr(start_idx - first_index, len)});

  std::vector<Interval> merged;
  auto it = buffer_.begin();
  Interval last = *it;
  it++;

  while (it != buffer_.end()) {
    if (it->start_ <= last.end_) {
      if (last.end_ < it->end_) {
        last.end_ = it->end_;
        last.data_ = last.data_.substr(0, it->start_ - last.start_) + it->data_;
      }
    } else {
    
      merged.push_back(last);
      last = *it;
    }
    it++;
  }
  merged.push_back(last);
  buffer_.clear();
  for (const auto& interval : merged) {
    buffer_.insert(interval);
  }

  it = buffer_.begin();
  while (it->start_ == nxt_assembled_idx_) {
    writer.push(it->data_);
    it = buffer_.erase(it);
  }

  if (nxt_assembled_idx_ == EOF_idx_) {
    writer.close();
  
  }

}

uint64_t Reassembler::bytes_pending() const
{
  uint64_t pending_size{0};
  for (const auto& interval : buffer_) {
    pending_size += interval.end_ - interval.start_;
  }

  return pending_size;
}
