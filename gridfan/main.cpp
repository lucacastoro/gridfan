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

#include "temperature.cpp"
#include "libgridfan.hpp"

using namespace std::chrono_literals;

static bool stop = false;
static void sig_handler(int) {
  stop = true;
}

template <typename T>
static T clamp( const T& min, const T& max, const T& val ) {
  return std::min( max, std::max( min, val ) );
}

int main( int argc, char* argv[] ) {

  signal( SIGINT, &sig_handler );
  signal( SIGQUIT, &sig_handler );

  grid::controller controller;

  if( not controller ) {
    std::cerr << "no controller" << std::endl;
    return 1;
  }

  temperature::monitor monitor;

  if( not monitor ) {
    std::cerr << "no monitor" << std::endl;
    return 1;
  }

  const auto cpu = std::find_if(monitor.begin(), monitor.end(), [](const temperature::sensor& sensor){
    return sensor.getName() == "CPU Temperature";
  });

  if( cpu == monitor.end()) {
    std::cerr << "could not find CPU temperature" << std::endl;
    return 1;
  }

  // maps temp. to speed percentage: 25C = 0%, 70C = 100%
  const auto func = []( double temp ) -> int {
    constexpr auto min = 25.0; // deg
    constexpr auto max = 70.0; // deg
    return clamp( 0, 100, int( ( temp - min ) * 100.0 / ( max - min ) ) );
  };

  static constexpr auto interval = 1s;
  int last_p = -1;

  while( not stop ) {

    const auto t = cpu->getTemp();
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

      std::cout << "setting speed to " << last_p << "%" << std::endl;

      for (auto& fan : controller) {
        fan.setPercent( last_p );
      }
    }

    std::this_thread::sleep_for(interval);
  }
}
