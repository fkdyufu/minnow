#include "reassembler.hh"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <vector>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  //将超出capacity部分删除，不管是首部或者是尾部
  //判断和相邻的字符串有无重叠，将重叠的字符串取出，组合成一个字符串，再插入map中
  //判断第一条string有没有和字节流中的最后一个接壤，有的话write
  //防御式编程，判断是否超出了末尾index
  uint64_t first_unassembled_index = output_.writer().bytes_pushed();
  uint64_t first_unacceptable_index = first_unassembled_index + output_.writer().available_capacity();
  uint64_t end_index = first_index + data.size();
  if(first_index < first_unassembled_index) {
    uint64_t erase_count = first_unassembled_index - first_index;
    data.erase(0, erase_count);
    first_index += erase_count;
  }
  if(first_unacceptable_index < end_index) {
    uint64_t erase_count = end_index - first_unacceptable_index;
    data.erase(data.size() - erase_count);
    end_index -= erase_count;
  }
  if ( is_last_substring ) {
    is_last_substring_flag = {true, first_index + data.size()};
  }
  // // 当前map中的string都没有重叠，需要取出当前可能重叠的string
  // if( !substrings.empty() ) {
  //   auto start = substrings.lower_bound(first_index);
  //   if (start != substrings.begin())
  //     --start;
  //   auto end = substrings.upper_bound(end_index);

  //   for (auto iter = start; iter != end; ++iter) {
  //     uint64_t left = std::min(iter->first, first_index);
  //     uint64_t right = std::max(iter->first + iter->second.size(), end_index);
  //     if (right - left <= iter->second.size() + data.size()) { //可以组合成一个字符串
  
  //     }
  //   }
  // }

  substrings.insert({first_index, data});
  while(!substrings.empty()) {
    auto [key, value] = *substrings.begin();
    if ( key <= first_unassembled_index ) {
      if ( key + value.size() > first_unacceptable_index) {
        if (!output_.writer().is_closed()) {
          output_.writer().push(value.substr(key + value.size()));
        } else {
          cerr << "The output ByteStream is closed!" << endl;
          // return ;
        }
      }
      substrings.erase(key);
    }
    first_unassembled_index = output_.writer().bytes_pushed();
  }


}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_pending_;
}
