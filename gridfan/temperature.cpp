#include "temperature.hpp"
#include <cassert>
#include <iostream>

namespace temperature {

  size_t monitor::ref_count = 0;

  sensor::sensor( const sensors_chip_name* c, const sensors_feature* f )
    : chip( c )
    , fea( f )
  {}

  double sensor::get( sensors_subfeature_type tp ) const 
  {
    double value = 0.0;
    const auto sub = sensors_get_subfeature( chip, fea, tp ); 

    if( sub and ( sub->flags & SENSORS_MODE_R ) )
    {
      const auto x = sensors_get_value(
        chip, 
        sub->number,
        &value
      );    

      assert( 0 == x );
    }

    return value;
  }

  double sensor::getTemp() const
  {
    return get( SENSORS_SUBFEATURE_TEMP_INPUT );
  }

  double sensor::getHigh() const 
  {
    return get( SENSORS_SUBFEATURE_TEMP_MAX );
  }

  double sensor::getCrit() const
  {
    return get( SENSORS_SUBFEATURE_TEMP_CRIT );
  }

  std::string sensor::getName() const
  {
    auto label = sensors_get_label( chip, fea );

    if( label )
    {
      std::string tmp( label );

      if( label != fea->name )
        free( label );

      return tmp;
    }

    return fea->name;
  }

  static monitor* g_monitor = nullptr;

  monitor::monitor()
  {
    if( 0 == ref_count++ )
    {
      if( 0 != sensors_init( nullptr ) )
      {
        ref_count = 0;
        return;
      }

      g_monitor = this;
    }

    int nr = 0;

    for( auto chip = sensors_get_detected_chips( nullptr, &nr ); chip ; chip = sensors_get_detected_chips( nullptr, &nr ) )
    {
      int ft = 0;

      for( auto feature = sensors_get_features( chip, &ft ); feature ; feature = sensors_get_features( chip, &ft ) )
      {
        if( SENSORS_FEATURE_TEMP == feature->type )
        {
          sensors.emplace_back( chip, feature );
        }
      }
    }
  }

  monitor::operator bool() const {
    return this == g_monitor;
  }

  const char* monitor::version() const
  {
    return libsensors_version;
  }

  monitor::~monitor()
  {
    if( ref_count and 0 == --ref_count )
    {
      sensors_cleanup();
      g_monitor = nullptr;
    }
  }
  
  size_t monitor::size() const
  { return sensors.size(); }
  bool monitor::empty() const
  { return sensors.empty(); }

  const sensor& monitor::operator [] ( size_t index ) const
  { return sensors[ index ]; }

  sensor& monitor::operator [] ( size_t index )
  { return sensors[ index ]; }

  monitor::iterator monitor::begin()
  { return sensors.begin(); }
  monitor::iterator monitor::end()
  { return sensors.end(); }
  monitor::const_iterator monitor::begin() const
  { return sensors.begin(); }
  monitor::const_iterator monitor::end() const
  { return sensors.end(); }

  monitor& global_monitor()
  {
    return *g_monitor;
  }
}
