#ifndef SERIAL_HPP
#define SERIAL_HPP

#include <iostream>
#include <utility>
#include <cmath>
#include <cstring>
#include <string>
#include <cassert>
#include <mutex>
#include <stdexcept>
#include <chrono>

#include "serial.h"

using namespace std::chrono_literals;

namespace serial
{
	typedef uint32_t baudrate_t;
	typedef uint32_t databits_t;
	typedef float stopbits_t;

  enum class parity_t {
		none  = PARITY_NONE,
		odd   = PARITY_ODD,
		even  = PARITY_EVEN,
		mark  = PARITY_MARK,
		space = PARITY_SPACE
	};

	class configuration
	{
	public:

		configuration()
		{ memset( &config, 0, sizeof(config) ); }

		configuration& baudrate( baudrate_t baudrate )
		{
			config.baudrate = baudrate;
			return *this;
		}

		configuration& parity( parity_t parity )
		{
      config.parity = uint32_t(parity);
			return *this;
		}

		configuration& stopbits( stopbits_t stop )
		{
			if( almost( stop, 1.0 ) ){ config.stopbits = STOPBIT_ONE; return *this; }
			if( almost( stop, 1.5 ) ){ config.stopbits = STOPBIT_ONE_HALF; return *this; }
			if( almost( stop, 2.0 ) ){ config.stopbits = STOPBIT_TWO; return *this; }
			assert( false );
			return *this;
		}

		configuration& databits( databits_t count )
		{
			assert( count > 4 and count < 10 );
			config.databits = count - ( DATABITS_9 - 1 );
			return *this;
		}

		uint32_t baudrate() const
		{ return config.baudrate; }

		parity_t parity() const
		{ return parity_t( config.parity ); }

		stopbits_t stopbits() const
		{
			switch( config.stopbits )
			{
				case STOPBIT_ONE: return 1.0;
				case STOPBIT_ONE_HALF: return 1.5;
				case STOPBIT_TWO: return 2.0;
			}

			assert( false );
		}

		databits_t databits() const
		{ return ( DATABITS_5 - 1 ) + config.databits; }

		const serial_config_t* operator * () const
		{ return &config; }

		static configuration make8N1( baudrate_t brate )
		{
      return configuration()
          .databits( 8 )
          .parity( serial::parity_t::none )
          .stopbits( 1 )
          .baudrate( brate );
		}

	private:

    static bool almost( float a, float b, float delta = -1.0F )
		{
      const auto diff = std::abs( a - b );

      if( 0.0 == diff )
        return true;

      if( delta < 0.0F )
        delta = diff / 100.0F;

      return delta > diff;
		}

		serial_config_t config;
	};

	typedef configuration config;

	class read_result
	{
	public:
		enum status { ok, error, timeout };
		static read_result success( size_t sz ){ return read_result( ok, sz ); }
		static read_result failure( status st ){ return read_result( st, 0 ); }
		inline operator bool() const { return ok == status; }
		status status;
		size_t amount;
	private:
		read_result( enum status st, size_t sz ) : status( st ), amount( sz ) {}
	};

  static constexpr auto infinite = std::chrono::milliseconds::max();
  static constexpr auto use_global = std::chrono::milliseconds::min();

	class file
	{
	public:

    using clock = std::chrono::steady_clock;

		file() noexcept
			: handle( INVALID_SERIAL )
		{}

		file( const char* filename, const configuration& config ) noexcept
			: handle( serial_open( filename, *config ) )
      , timeout(infinite)
		{}

		file( file&& other ) noexcept
			: file()
		{
			(*this) = std::move( other );
		}

		file& operator = ( file&& other ) noexcept
		{
      const std::lock_guard<std::mutex> lock1( mutex );
      const std::lock_guard<std::mutex> lock2( other.mutex );

      if( this != &other )
      {
        serial_close( handle );
        this->handle = other.handle;
        this->last_read = other.last_read;
        this->last_write = other.last_write;
        this->timeout = other.timeout;
        other.handle = INVALID_SERIAL;
      }

			return *this;
		}

    virtual ~file() noexcept
		{
      const std::lock_guard<std::mutex> lock( mutex );
      serial_close( handle );
		}

		explicit operator bool () const noexcept
		{ return INVALID_SERIAL != handle; }

		bool write( const void* data, size_t count ) noexcept
		{
      const std::lock_guard<std::mutex> lock( mutex );

      const auto success = serial_write( handle, data, count );
      last_write = clock::now();

      if( success )
        return true;

      std::cerr << "serial::write error: " << strerror(errno) << std::endl;
      return false;
		}

		bool write( const char* data ) noexcept
		{ return write( data, strlen( data ) ); }

		bool write( const std::string& string ) noexcept
		{ return write( string.c_str(), string.size() ); }

    read_result read( void* data, size_t count, const std::chrono::milliseconds& to = use_global ) noexcept
		{
      const std::lock_guard<std::mutex> lock( mutex );

			if( timeout < 0s )
				return read_result::failure( read_result::timeout );

      const auto x = to == use_global ? timeout : to;

      const auto success = serial_read( handle, data, &count, x == infinite ? NO_TIMEOUT : uint32_t( x.count() ) );

      last_read = clock::now();

      if (success)
        return read_result::success( count );

      if(ETIME == errno)
        return read_result::failure( read_result::timeout );

      return read_result::failure( read_result::error );
		}

    read_result read_all( void* data, size_t count, const std::chrono::milliseconds& to = use_global ) noexcept
		{
      const std::lock_guard<std::mutex> lock( mutex );

			if( timeout.count() < 0 )
				return read_result::failure( read_result::timeout );

      const auto x = to == use_global ? timeout : to;

      const auto success = serial_read_all( handle, data, count, x == infinite ? NO_TIMEOUT : uint32_t( x.count() ) );
      last_read = clock::now();

      if (success)
        return read_result::success( count );

      if(ETIME == errno)
        return read_result::failure( read_result::timeout );

      return read_result::failure( read_result::error );
		}

		template<typename type_t>
    inline file& write( const type_t& some ) noexcept(false)
		{
			if( not write( &some, sizeof(some) ) )
				throw std::runtime_error( strerror( errno ) );
			return *this;
		}

		template<typename type_t>
		typename std::enable_if<std::is_trivially_copyable<type_t>::value, file&>::type
    inline read( type_t& some, const std::chrono::milliseconds& timeout = use_global ) noexcept(false)
		{
			if( not read_all( &some, sizeof(some), timeout ) )
				throw std::runtime_error( strerror( errno ) );
			return *this;
		}

		template<typename type_t>
		typename std::enable_if<std::is_trivially_copyable<type_t>::value, type_t>::type
    inline read( const std::chrono::milliseconds& timeout = use_global ) noexcept(false)
		{
			type_t some;
      if( not read( some, timeout ) )
        throw std::runtime_error( strerror( errno ) );
			return some;
		}

		template<typename type_t>
    inline file& operator << ( const type_t& some ) noexcept(false)
		{
			return write( some );
		}

		template<typename type_t>
    inline file& operator >> ( type_t& some ) noexcept(false)
		{
			return read( some );
		}

    clock::time_point get_last_read() const noexcept
    {
      return last_read;
    }

    clock::time_point get_last_write() const noexcept
    {
      return last_write;
    }

    clock::time_point get_last_access() const noexcept
    {
      return std::max(last_read, last_write);
    }

    void set_timeout( const std::chrono::milliseconds& to )
    {
      const std::lock_guard<std::mutex> lock( mutex );
      timeout = to;
    }

    const std::chrono::milliseconds& get_timeout() const
    {
      const std::lock_guard<std::mutex> lock( mutex );
      return timeout;
    }

	private:

		file( const file& ) = delete;
		file& operator = ( const file& ) = delete;

		serial_t handle;
    mutable std::mutex mutex;
    std::chrono::milliseconds timeout;
    clock::time_point last_read;
    clock::time_point last_write;
	};
}

#endif // SERIAL_HPP
