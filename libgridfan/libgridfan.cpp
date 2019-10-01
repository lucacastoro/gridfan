#include "libgridfan.hpp"

#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <thread>
#include <algorithm>
#include <chrono>

#include <iomanip>

namespace grid {

  static constexpr auto delay_between_access = 50ms;
  static constexpr uint8_t PING        = 0xC0;
  static constexpr uint8_t PING_OK     = 0x21;
  static constexpr uint8_t GET_UNKN1   = 0x84;
  static constexpr uint8_t GET_UNKN2   = 0x85;
  static constexpr uint8_t GET_RPM     = 0x8A;
  static constexpr uint8_t SET_VOLTAGE = 0x44;

  controller::controller(std::nothrow_t, const std::string& filename) noexcept(false)
    : file( filename.c_str(), serial::configuration::make8N1( 4800 ) )
  {
    if( not file )
    {
      return;
    }

    if( result_t::ok != init( 5s ) )
    {
      file.close();
      return;
    }

    file.set_timeout(5s);

    for( size_t i = 0; i < fans.size(); ++i )
      fans[ i ] = fan( file, i + 1 );
  }

  controller::controller( const std::string& filename ) noexcept(false)
    : controller(std::nothrow, filename)
	{
    if( not file )
      throw std::runtime_error("Could not access " + filename);
	}

	controller::operator bool() const
	{
		return bool( file );
	}

	size_t controller::size() const
	{
		return fans.size();
	}

	bool controller::empty() const
	{
		return fans.empty();
	}

	controller::iterator controller::begin()
	{
		return fans.begin();
	}

	controller::iterator controller::end()
	{
		return fans.end();
	}

	controller::const_iterator controller::begin() const
	{
		return fans.begin();
	}

	controller::const_iterator controller::end() const
	{
		return fans.end();
	}

	fan& controller::operator [] ( size_t index )
	{
		return fans[ index ];
	}

	const fan& controller::operator [] ( size_t index ) const
	{
		return fans[ index ];
	}

  controller::result_t controller::init( const std::chrono::milliseconds& timeout )
	{
    using clock = std::chrono::steady_clock;

    const auto end = clock::now() + timeout;
    const auto step = 200ms;

    while( true )
    {
      const auto x = file.write( PING ).read<uint8_t>( 100ms );

      if( PING_OK == x )
        return result_t::ok;

      const auto now = clock::now();

      if( now > end )
        break;

      std::this_thread::sleep_until(std::min(now + step, end));
    }

    return result_t::timeout;
	}

  controller::result_t controller::ping( const std::chrono::milliseconds& timeout )
	{
    std::this_thread::sleep_until(file.get_last_access() + delay_between_access);
    const auto x = file.write( PING ).read<uint8_t>( timeout );
    return ( PING_OK != x ) ? result_t::ok : result_t::invalid_data;
	}

	fan::fan()
		: file( nullptr )
    , index( 0 )
	{}

	fan::fan(serial::file &f, size_t i )
		: file( &f )
		, index( i )
	{}

  fan::fan( fan&& o )
    : fan()
  {
    (*this) = std::move(o);
  }

  fan& fan::operator = (fan&& o )
  {
    if( this != &o )
    {
      index = o.index;
      file = o.file;
    }
    return *this;
  }

	fan::operator bool () const
	{
		return ( nullptr != file );
	}

	size_t fan::id() const
	{
		return index;
	}

  int fan::get( uint8_t v, const std::chrono::milliseconds& timeout ) const noexcept(false)
  {
    const uint8_t command[] = { v, uint8_t(index) };
    uint8_t answer[5];

    std::this_thread::sleep_until(file->get_last_access() + delay_between_access);
    if( not file->write( command, sizeof(command) ) )
      throw std::runtime_error( strerror( errno ) );

    std::this_thread::sleep_until(file->get_last_access() + delay_between_access);
    if( not file->read( answer, sizeof(answer), timeout ) )
      throw std::runtime_error( strerror( errno ) );

    // The reply should be something like "C0 00 00 02 76".
    //   "C0 00 00" is always the same and the last two bytes are the fan speed RPM in hex.

    if( answer[0] != 0xc0
    or  answer[1] != 0x00
    or  answer[2] != 0x00
    )
    {
      throw std::runtime_error("unexpected data");
    }

    return uint16_t( ( uint16_t( answer[3] ) << 8 ) | uint16_t( answer[4] ) );
  }

  int fan::getSpeed( const std::chrono::milliseconds& timeout ) const noexcept(false)
	{
    return get(GET_RPM, timeout);
	}

  int fan::getUnknown1( const std::chrono::milliseconds& timeout ) const noexcept(false)
  {
    return get(GET_UNKN1, timeout);
  }

  int fan::getUnknown2( const std::chrono::milliseconds& timeout ) const noexcept(false)
  {
    return get(GET_UNKN2, timeout);
  }

  void fan::setPercent( int pr )
	{
    if( pr < 0 or pr > 100 )
      throw std::runtime_error("invalid percent value: " + std::to_string(pr));

    uint8_t raw = uint8_t( 4 + std::min( 8, ( pr - 20 ) * 8 / 75 ) );

    // la velocita' va da 0 a 12
    // non puo' essere maggiore di 0 e minore di 4
    // quindi se == 1 lo trasformo in zero
    // se 2 o 3 lo trasformo in 4

    if( raw < 4 ) raw = 4;
    if( raw < 2 ) raw = 0;

		const uint8_t command[7] = {
      SET_VOLTAGE, uint8_t(index), 0xc0 , 0, 0, raw, 0
		};

    std::this_thread::sleep_until(file->get_last_access() + delay_between_access);
    if( not file->write( command ) )
      throw std::runtime_error("I/O error");

    std::this_thread::sleep_until(file->get_last_access() + delay_between_access);
    const auto result = file->read<uint8_t>();

    if( result != 0x01 )
      throw std::runtime_error("invalid data");
	}
}
