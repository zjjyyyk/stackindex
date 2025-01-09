#pragma once

#include <array>
#include <chrono>

enum struct TIMER : size_t {
  UPDATE, EVALUATE, PUSH, ADAPT, REFINE, BUILD, OUTPUT, _
};

class Timer {
private:
  static std::array<double, (size_t)TIMER::_> timers;

public:
  static double used(TIMER timer) {
    return timers[(size_t)timer];
  }

  static void reset(TIMER timer) {
    timers[(size_t)timer] = 0;
  }

  static void reset_all() {
    for (size_t id = 0; id < (size_t)TIMER::_; ++id) timers[id] = 0;
  }

private:
  using clock = std::chrono::steady_clock;
  using duration = std::chrono::duration<double>;

  const size_t _timer_id;
  const clock::time_point _setup_time;

public:
  Timer(TIMER timer) :
      _timer_id((size_t)timer),
      _setup_time(clock::steady_clock::now()) { }

  ~Timer() {
    timers[_timer_id] +=
      std::chrono::duration_cast<duration>(clock::now() - _setup_time).count();
  }
};

std::array<double, (size_t)TIMER::_> Timer::timers { };
