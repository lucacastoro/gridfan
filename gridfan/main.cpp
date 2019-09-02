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
#include <syslog.h>

#include "temperature.hpp"
#include "libgridfan.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

static bool stop = false;
static int got_signal = 0;
static void sig_handler(int sig) {
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

class Logger {
public:
  virtual ~Logger() = default;
  template <typename ...Args>
  inline void info(Args&&... args) {
    log(LOG_INFO, std::forward<Args>(args)...);
  }
  template <typename ...Args>
  inline void warning(Args&&... args) {
    log(LOG_WARNING, std::forward<Args>(args)...);
  }
  template <typename ...Args>
  inline void error(Args&&... args) {
    log(LOG_ERR, std::forward<Args>(args)...);
  }
private:
  virtual void log(int pri, const char* msg) = 0;
  template <typename ...Args>
  inline void log(int pri, const char* fmt, const Args& ...args) {
    char temp[1024];
    const auto l = snprintf(temp, sizeof(temp), fmt, args...);
    if (l > -1) {
      temp[l] = 0;
      log(pri, temp);
    }
  }
};

class SysLog final : public Logger {
public:
  SysLog() {
    openlog(nullptr, 0, 0);
  }
  virtual ~SysLog() override {
    closelog();
  }
private:
  void log(int pri, const char* msg) override {
    syslog(pri, "%s", msg);
  }
};

class LocalLog final : public Logger {
public:
  virtual ~LocalLog() override = default;
private:
  static inline const char* prefix(int pri) {
    return pri == LOG_ERR ?     "[ERROR]" :
           pri == LOG_WARNING ? "[WARN.]" :
           pri == LOG_INFO ?    "[INFO.]" :
                                "[?????]";
  }
  void log(int pri, const char* msg) override {
    std::cout << prefix(pri) << " " << msg << std::endl;
  }
};

static inline double linear( double temp ) {
  constexpr auto min = 25.0; // deg
  constexpr auto max = 70.0; // deg
  return clamp( 0, 100, int( ( temp - min ) * 100.0 / ( max - min ) ) );
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

  std::unique_ptr<Logger> log;

  if (false) {
    log = std::make_unique<SysLog>();
  } else {
    log = std::make_unique<LocalLog>();
  }

  grid::controller controller;

  if( not controller ) {
    log->error("cannot access the fan controller");
    return 1;
  }

  temperature::monitor monitor;

  if( not monitor ) {
    log->error("cannot access the temperature monitor");
    return 1;
  }

  const auto cpu = monitor.find("CPU Temperature");

  if( cpu == monitor.end()) {
    log->error("cannot find the CPU temperature sensor");
    return 1;
  }

  log->info("started");

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

        log->info("setting fans speed to %d%%", last_p);

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
        log->error("exception caught: %s", ex.what());
        log->error("too many errors, giving up");
        stop = true;
      }
      else
      {
        log->warning("exception caught: %s", ex.what());

        interruptible_sleep(5s);
        if (stop) break;

        controller = grid::controller();

        if( not controller )
        {
          log->error("could not re-initialize the controller");
          stop = true;
        }
      }
    }
  }

  if (got_signal)
    log->info("got signal '%s' (%d)", strsignal(got_signal), got_signal);

  log->info("terminated");
}
