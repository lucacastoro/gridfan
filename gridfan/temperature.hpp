#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <vector>
#include <string>
#include <sensors/sensors.h>

namespace temperature {

  class sensor {
  public:
    sensor( const sensors_chip_name* c, const sensors_feature* f );
    double getTemp() const;
    double getHigh() const;
    double getCrit() const;
    std::string getName() const;
  private:
    double get( sensors_subfeature_type tp ) const;
    const sensors_chip_name* chip;
    const sensors_feature* fea;
  };

  class monitor final {
  public:

    monitor();
    ~monitor();

    explicit operator bool() const;

    const char* version() const;

    size_t size() const;
    bool empty() const;

    typedef std::vector<sensor>::iterator iterator;
    typedef std::vector<sensor>::const_iterator const_iterator;

    const sensor& operator [] ( size_t ) const;
    sensor& operator [] ( size_t );

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const; 

  private:
    static size_t ref_count;
    std::vector<sensor> sensors;
  };

  monitor& global_monitor();
}

#endif // TEMPERATURE_H

