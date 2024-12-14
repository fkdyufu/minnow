#include "reassembler.hh"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <vector>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  (void) is_last_substring;
  auto writer = output_.writer();

  //清除data首尾两端超出的部分
  uint64_t first_unassembled_index = writer.bytes_pushed();
  uint64_t first_unacceptable_index = writer.available_capacity() + first_unassembled_index;
  if( first_index < first_unassembled_index ) {
    data.erase(0, first_unassembled_index - first_index);
    first_index = first_unassembled_index;
  }
  if ( first_index + data.size() > first_unacceptable_index ) {
    data.erase(first_unacceptable_index - first_index);
  }

  //找出最小的可能重叠的字符串
  auto it = substrings.lower_bound(first_index);
  if ( it != substrings.begin() && prev(it)->first + prev(it)->second.size() >= first_index ) {
    --it;
  }

  //合并所有重叠的字符串
  while(it != substrings.end() && it->first <= first_index + data.size()) {
    uint64_t new_first = min(it->first, first_index);
    uint64_t new_last = max(it->first + it->second.size(), first_index + data.size());

    string merge(new_last - new_first, '\0');
    copy(it->second.begin(), it->second.end(), merge.begin());
    copy(data.begin(), data.end(), merge.begin() + (first_index - it->first));

    first_index = new_first;
    data = std::move(merge);
    bytes_pending_ -= it->second.size();
    it = substrings.erase(it);
  }
  substrings[first_index] = std::move(data);
  bytes_pending_ += substrings[first_index].size();

cout << "s.f " << substrings.begin()->first << endl;
cout << "s.s " << substrings.begin()->second << endl;
  //push
  if(writer.bytes_pushed() == substrings.begin()->first) {
    writer.push(substrings.begin()->second);
    bytes_pending_ -= substrings.begin()->second.size();
    substrings.erase(substrings.begin());
  }
  cout << "==============bytes_pushed===========:\n" << writer.bytes_pushed() << endl;
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_pending_;
}
