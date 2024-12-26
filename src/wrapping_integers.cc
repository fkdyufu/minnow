#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 { zero_point + static_cast<uint32_t>( n ) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint64_t base = checkpoint & ~( static_cast<uint64_t>( UINT32_MAX ) );
  uint64_t candidates[3]
    = { base == 0 ? base : base - ( 1UL << 32 ), base, base + ( 1UL << 32 ) == 0 ? base : base + ( 1UL << 32 ) };
  uint64_t best_seqno = 0, min_diff = UINT64_MAX;
  for ( const uint64_t candidate : candidates ) {
    uint64_t absolute_seqno = candidate + ( raw_value_ - zero_point.raw_value_ );
    uint64_t diff
      = ( absolute_seqno < checkpoint ) ? ( checkpoint - absolute_seqno ) : ( absolute_seqno - checkpoint );
    if ( diff < min_diff ) {
      min_diff = diff;
      best_seqno = absolute_seqno;
    }
  }
  return best_seqno;
}
