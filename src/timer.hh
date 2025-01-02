#pragma once

#include <cstdint>
class Timer
{
public:
  bool is_running();
  void start( uint64_t RTO );
  void stop();
  void past( uint64_t time_ms );
  bool is_expired();
  Timer();

private:
  bool is_running_;
  uint64_t RTO_;
  uint64_t current_time_ms_;
};