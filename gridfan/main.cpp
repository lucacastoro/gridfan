#include <iostream>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <unordered_map>
#include <numeric>
#include <memory>
#include <variant>
#include <csignal>
#include <cmath>
#include <ctime>

#include "temperature.hpp"
#include "libgridfan.hpp"
#include "logger.hpp"

#include "yaml-cpp/yaml.h"
#include "gflags/gflags.h"

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

class algo {
public:
  virtual double operator () (double) = 0;
protected:
  template <typename T>
  T get(const YAML::Node& conf, const char* name, T def) {
    if (conf[name]) return conf[name].as<T>();
    return def;
  }
};

class algo_linear : public algo {
public:
  algo_linear(const YAML::Node& conf) {
    min_tmp = get(conf, "min_temp", 25.0);   // deg
    max_tmp = get(conf, "max_temp", 70.0);   // deg
    min_spd = get(conf, "min_speed", 10.0);  // %
    max_spd = get(conf, "max_speed", 100.0); // %
  }

  virtual double operator () (double temp) override final {
    return clamp( min_spd, max_spd, ( temp - min_tmp ) * 100.0 / ( max_tmp - min_tmp ) );
  }
private:
  double min_tmp;
  double max_tmp;
  double min_spd;
  double max_spd;
};

static inline double softplus( double x )
{
  return log1p( exp( x ) );
}

static inline double logistic( double x )
{
  return 1.0 / ( 1.0 + exp( -x ) );
}

#define DEFAULT_CONFIG_FILE "/etc/gridfan/config.yml"
#define DEFAULT_LOGGER "syslog"
#define DEFAULT_ALGORITHM "linear"

DEFINE_string(config, DEFAULT_CONFIG_FILE, "configuration file");
DEFINE_string(logger, DEFAULT_LOGGER, "logger");
DEFINE_string(algorithm, DEFAULT_ALGORITHM, "algorithm");

int main(int argc, char** argv) {

  signal(SIGINT, &sig_handler);
  signal(SIGQUIT, &sig_handler);
  signal(SIGTERM, &sig_handler);
  signal(SIGUSR1, &sig_handler);

  gflags::ParseCommandLineFlags(&argc, &argv, true);

  const std::string config_file = FLAGS_config.size() ? FLAGS_config : DEFAULT_CONFIG_FILE;

  const auto load_config = [](const std::string& file) -> YAML::Node {
    try {
      return YAML::LoadFile(file);
    } catch (std::exception& ex) {
      return {};
    }
  };

  const YAML::Node config = load_config(config_file);

  if (not config.size() and FLAGS_config.size()) {
    std::cerr << "cannot open " << FLAGS_config << std::endl;
    return 1;
  }

  const auto get_conf = [&config](const char* name, const std::string& flag, const char* def) -> std::string {
    if (flag.size()) return flag;
    if (config[name]) return config[name].as<std::string>();
    return def;
  };

  const auto log_type = get_conf("logger", FLAGS_logger, DEFAULT_LOGGER);
  const auto algo_type = get_conf("algorithm", FLAGS_algorithm, DEFAULT_ALGORITHM);

  std::unique_ptr<Logger> log;
  if (log_type == "syslog") {
    log = std::make_unique<SysLog>();
  } else if (log_type == "output") {
    log = std::make_unique<LocalLog>();
  } else {
    std::cerr << "invalid logger value" << std::endl;
    return 1;
  }

  const std::unique_ptr<algo> algorithm = algo_type == "linear"
    ? std::make_unique<algo_linear>(config)
    : nullptr;
  
  if (not algorithm) {
     log->error("invalid algorithm: %s", algo_type.c_str());
     return 1;
  }
  
  grid::controller controller(std::nothrow);

  if (not controller) {
    log->error("cannot access the fan controller");
    return 1;
  }

  temperature::monitor monitor;

  if (not monitor) {
    log->error("cannot access the temperature monitor");
    return 1;
  }

  const auto cpu = monitor.find("CPU Temperature");

  if (cpu == monitor.end()) {
    log->error("cannot find the CPU temperature sensor");
    return 1;
  }

  log->info("applying %s algorithm", algo_type.c_str());

  static constexpr milliseconds interval = 1s;
  int last_p = -1;

  size_t errors = 0;
  constexpr size_t max_errors = 5;

  while (not stop) {

    try {
      const auto t = cpu->temperature();
      const auto p = int( (*algorithm)( t ) );

      if (verbose_trigger) {
        verbose_trigger = false;
        verbose = not verbose;
        log->info("verbose mode %s", verbose ? "activated" : "deactivated");
        if (verbose) {
          log->info("current temperature is %.2f degree", t);
          log->info("current speed is %d%%", p);
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
          log->info("cpu is %.2f degree, setting fans speed to %d%%", t, last_p);
        }

        for (auto& fan : controller) {
          fan.setPercent( last_p );
        }
      }

      errors = 0;
      interruptible_sleep(interval);
      if (stop) break;
    }
    catch( const std::exception& ex ) {
      if( ++errors == max_errors ) {
        log->error("exception caught: %s", ex.what());
        log->error("too many errors, giving up");
        stop = true;
      }
      else {
        log->warning("exception caught: %s", ex.what());

        interruptible_sleep(5s);
        if (stop) break;

        controller = grid::controller();

        if( not controller ) {
          log->error("could not re-initialize the controller");
          stop = true;
        }
      }
    }
  }

  if (got_signal) {
    log->info("got signal '%s' (%d)", strsignal(got_signal), got_signal);
  }

  log->info("terminated");
}
