#include <iostream>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <unordered_map>
#include <numeric>
#include <csignal>
#include <cmath>
#include <ctime>

#include "temperature.hpp"
#include "libgridfan.hpp"
#include "logger.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

static bool stop = false;
static int got_signal = 0;
static bool verbose_trigger = false;
static bool verbose = false;

static void sig_handler(int sig) {
  if (SIGUSR1 == sig) {
    verbose_trigger = true;
    return;
  }
  stop = true;
  got_signal = sig;
}

template <typename R, typename P>
static std::chrono::duration<R,P> interruptible_sleep(const std::chrono::duration<R,P>& dur) {
  using namespace std::chrono;
  const auto sec = duration_cast<seconds>(dur).count();
  const auto nsec = (duration_cast<nanoseconds>(dur) - seconds(sec)).count();
  const struct timespec ts = {sec, nsec};
  struct timespec rm = {0, 0};
  const auto x = nanosleep(&ts, &rm);
  if (-1 == x and EINTR == errno) {
    return duration_cast<duration<R,P>>(seconds(rm.tv_sec) + nanoseconds(rm.tv_nsec));
  }
  return {};
}

template <typename T>
static T clamp( const T& min, const T& max, const T& val ) {
  return std::min( max, std::max( min, val ) );
}

static inline double linear( double temp ) {
  constexpr auto min_tmp = 25.0; // deg
  constexpr auto max_tmp = 70.0; // deg
  constexpr auto min_spd = 10;   // %
  constexpr auto max_spd = 100;  // %
  return clamp( min_spd, max_spd, int( ( temp - min_tmp ) * 100.0 / ( max_tmp - min_tmp ) ) );
};

static inline double softplus( double x )
{
  return log1p( exp( x ) );
}

static inline double logistic( double x )
{
  return 1.0 / ( 1.0 + exp( -x ) );
}

int main() {

  signal( SIGINT, &sig_handler );
  signal( SIGQUIT, &sig_handler );
  signal( SIGTERM, &sig_handler );
  signal( SIGUSR1, &sig_handler );

  using Log = SysLog;

  Log log;

  grid::controller controller(std::nothrow);

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

  using func_t = double (*) (double);
  func_t func = &linear;

  static constexpr milliseconds interval = 1s;
  int last_p = -1;

  size_t errors = 0;
  constexpr size_t max_errors = 5;

  while( not stop ) {

    try
    {
      const auto t = cpu->temperature();
      const auto p = int( func( t ) );

      if (verbose_trigger) {
        verbose_trigger = false;
        verbose = not verbose;
        log.info("verbose mode %s", verbose ? "activated" : "deactivated");
        if (verbose) {
          log.info("current temperature is %.2f degree", t);
          log.info("current speed is %d%%", p);
        }
      }

      // changes in fan speed are triggered only if either
      // - desired speed is higher than current speed
      //                     or
      // - desired speed is *way* lower than current speed (5%)

      if( p > last_p or last_p - p > 5 ) {
        // if desired speed is lower than current speed, slowly decrease it
        // at a maximum rate of -10% per second
        if( p < last_p ) {
          last_p = std::max( p, last_p - 10 );
        } else {
          last_p = p;
        }

        if (verbose) {
          log.info("setting fans speed to %d%%", last_p);
        }

        for (auto& fan : controller) {
          fan.setPercent( last_p );
        }
      }

      errors = 0;
      interruptible_sleep(interval);
      if (stop) break;
    }
    catch( const std::exception& ex )
    {
      if( ++errors == max_errors )
      {
        log.error("exception caught: %s", ex.what());
        log.error("too many errors, giving up");
        stop = true;
      }
      else
      {
        log.warning("exception caught: %s", ex.what());

        interruptible_sleep(5s);
        if (stop) break;

        controller = grid::controller();

        if( not controller )
        {
          log.error("could not re-initialize the controller");
          stop = true;
        }
      }
    }
  }

  if (got_signal)
    log.info("got signal '%s' (%d)", strsignal(got_signal), got_signal);

  log.info("terminated");
}
