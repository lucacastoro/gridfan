#ifndef LIBFANGRID_H
#define LIBFANGRID_H

#include "serial.hpp"
#include <array>
#include <memory>
#include <type_traits>
#include <vector>
#include <functional>

using namespace std::chrono_literals;

namespace grid
{
	class fan
	{
	public:

		typedef size_t id_t;

		fan();
    fan( serial::file& f, id_t index );
    fan(fan&& o );
    fan& operator = ( fan&& o );
		operator bool () const;
		id_t id() const;
    int getSpeed( const std::chrono::milliseconds &timeout = 500ms ) const noexcept(false);
    int getUnknown1( const std::chrono::milliseconds &timeout = 500ms ) const noexcept(false);
    int getUnknown2( const std::chrono::milliseconds &timeout = 500ms ) const noexcept(false);
    void setPercent( int );

	private:

    int get( uint8_t, const std::chrono::milliseconds &timeout = 500ms ) const noexcept(false);

    serial::file* file;
		id_t index;
	};

	class controller
	{
	public:
    explicit controller(const std::string& filename = "/dev/GridPlus0" ) noexcept(false);

    enum class result_t { ok, timeout, invalid_data };

		operator bool () const;

    typedef std::array<fan,6>::iterator iterator;
    typedef std::array<fan,6>::const_iterator const_iterator;

		size_t size() const;
		bool empty() const;

		fan& operator [] ( size_t index );
		const fan& operator [] ( size_t index ) const;

		iterator begin();
		iterator end();

		const_iterator begin() const;
		const_iterator end() const;

		iterator find( size_t id );
		const_iterator find( size_t id ) const;

	private:

    result_t init(const std::chrono::milliseconds& timeout );
    result_t ping( const std::chrono::milliseconds& timeout );

    std::array<fan,6> fans;
    serial::file file;
	};
}

template <typename ostream_t>
static inline ostream_t& operator << ( ostream_t& os, const grid::fan& fan)
{
  return os << "fan #" << fan.id(), os;
}

#endif // LIBFANGRID_H
