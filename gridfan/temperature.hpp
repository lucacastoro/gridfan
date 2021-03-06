#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <vector>
#include <string>
#include <algorithm>
#include <sensors/sensors.h>

namespace temperature {

  class sensor {
  public:
    sensor( const sensors_chip_name* c, const sensors_feature* f );
    double temperature() const;
    double high() const;
    double crit() const;
    std::string name() const;
  private:
    double get( sensors_subfeature_type tp ) const;
    const sensors_chip_name* chip;
    const sensors_feature* fea;
  };

  class monitor final {
  public:

    monitor() noexcept;
    ~monitor() noexcept;

    explicit operator bool() const;

    const char* version() const;

    size_t size() const noexcept;
    bool empty() const noexcept;

    typedef std::vector<sensor>::iterator iterator;
    typedef std::vector<sensor>::const_iterator const_iterator;

    const sensor& operator [] ( size_t ) const;
    sensor& operator [] ( size_t );

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;

    inline const_iterator find(const std::string& name) const {
      return std::find_if(
          begin(),
          end(),
            [&name](const auto& sensor){
            return sensor.name() == name;
        });
    }

  private:
    static size_t ref_count;
    std::vector<sensor> sensors;
  };

  monitor& global_monitor() noexcept;
}

#endif // TEMPERATURE_H

