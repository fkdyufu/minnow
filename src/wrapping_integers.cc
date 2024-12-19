#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 { zero_point + static_cast<uint32_t>(n) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint64_t max_uint32 = (1UL << 32) - 1;
  uint32_t groups = checkpoint >> 32;
  uint64_t min_diff = 1UL << 32;
  uint32_t left = groups > 0 ? groups - 1 : 0;
  uint32_t right = groups < max_uint32 ? groups + 1 : max_uint32;
  uint64_t absolute_seqno = (raw_value_ + (static_cast<uint64_t>(left) << 32)) 
                                - static_cast<uint64_t>(zero_point.raw_value_);
  for (uint32_t i = left; i <= right; ++i) {
    uint64_t absolute_seqno_temp = (raw_value_ + (static_cast<uint64_t>(i) << 32)) 
                                - static_cast<uint64_t>(zero_point.raw_value_);
    uint64_t diff = (absolute_seqno_temp < checkpoint) ? (checkpoint - absolute_seqno_temp)
                     : (absolute_seqno_temp - checkpoint);
    if(diff < min_diff) {
      min_diff = diff;
      absolute_seqno = absolute_seqno_temp;
    }
  }
  return absolute_seqno;
}
