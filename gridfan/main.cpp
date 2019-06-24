#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <csignal>
#include <unordered_map>
#include <numeric>
#include <syslog.h>
#include "temperature.hpp"
#include "libgridfan.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

static bool stop = false;
static void sig_handler(int) {
  stop = true;
}

template <typename T>
static T clamp( const T& min, const T& max, const T& val ) {
  return std::min( max, std::max( min, val ) );
}

class Syslog final {
public:
  inline Syslog() { openlog(nullptr, 0, 0); }
  inline ~Syslog() { closelog(); }
  template <typename ...Args>
  inline void info(const Args& ...args) {
    log(LOG_INFO, args...);
  }
  template <typename ...Args>
  inline void error(const Args& ...args) {
    log(LOG_ERR, args...);
  }
private:
  template <typename ...Args>
  inline void log(int pri, const Args& ...args) {
    syslog(pri, args...);
  }
};

int main() {

  signal( SIGINT, &sig_handler );
  signal( SIGQUIT, &sig_handler );

  Syslog log;

  grid::controller controller;

  if( not controller ) {
    log.error("cannot access the fan controller");
    return 1;
  }

  temperature::monitor monitor;

  if( not monitor ) {
    log.error("cannot access the temperature monitor");
    return 1;
  }

  const auto cpu = monitor.find("CPU Temperature");

  if( cpu == monitor.end()) {
    log.error("cannot find the CPU temperature sensor");
    return 1;
  }

  log.info("started");

  // maps temp. to speed percentage: 25C = 0%, 70C = 100%
  const auto func = []( double temp ) -> int {
    constexpr auto min = 25.0; // deg
    constexpr auto max = 70.0; // deg
    return clamp( 0, 100, int( ( temp - min ) * 100.0 / ( max - min ) ) );
  };

  static constexpr milliseconds interval = 1s;
  int last_p = -1;

  while( not stop ) {

    const auto t = cpu->temperature();
    const auto p = func( t );

    // changes in fan speed are triggered only if
    // - desired speed is higher than current speed
    // - desired speed is *way* lower than current speed (5C)

    if( p > last_p or last_p - p > 5 ) {
      // if desired speed is lower than current speed, slowly decrease it
      // at a maximum rate of -10% per second
      if( p < last_p ) {
        last_p = std::max( p, last_p - 10 );
      } else {
        last_p = p;
      }

      log.info("setting fans speed to %d%%", last_p);

      for (auto& fan : controller) {
        fan.setPercent( last_p );
      }
    }

    for (size_t i = 0; i < 10; ++i) {
      std::this_thread::sleep_for(interval / 10);
      if (stop) break;
    }
  }

  log.info("terminated");
}
