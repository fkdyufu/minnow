#include "timer.hh"

bool Timer::is_running()
{
  return is_running_;
}

void Timer::start( uint64_t RTO )
{
  RTO_ = RTO;
  is_running_ = true;
  current_time_ms_ = 0;
}

void Timer::stop()
{
  is_running_ = false;
}

void Timer::past( uint64_t ms )
{
  if ( is_running_ )
    current_time_ms_ += ms;
}

bool Timer::is_expired()
{
  return current_time_ms_ >= RTO_;
}

Timer::Timer() : is_running_( false ), RTO_( 0 ), current_time_ms_( 0 ) {}