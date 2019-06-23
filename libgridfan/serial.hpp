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

	enum parity {
		none  = PARITY_NONE,
		odd   = PARITY_ODD,
		even  = PARITY_EVEN,
		mark  = PARITY_MARK,
		space = PARITY_SPACE
	};

	typedef parity parity_t;

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
			config.parity = parity;
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
			return configuration().databits( 8 ).parity( serial::parity::none ).stopbits( 1 ).baudrate( brate );
		}

	private:

		static bool almost( float a, float b, float delta = 0.01F )
		{
			return std::abs( a - b ) < delta;
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

	class file
	{
	public:

    using clock = std::chrono::steady_clock;

		file() noexcept
			: handle( INVALID_SERIAL )
		{}

		file( const char* filename, const configuration& config ) noexcept
			: handle( serial_open( filename, *config ) )
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

			if( this == &other ) return *this;
			if( *this ) serial_close( handle );
			this->handle = other.handle;
			other.handle = INVALID_SERIAL;
			return *this;
		}

		virtual ~file()
		{
      const std::lock_guard<std::mutex> lock( mutex );

			if( *this )
			{
				serial_close( handle );
			}
		}

		explicit operator bool () const noexcept
		{ return INVALID_SERIAL != handle; }

		bool write( const void* data, size_t count ) noexcept
		{
      const std::lock_guard<std::mutex> lock( mutex );
      last_access = clock::now();
      const ssize_t w = serial_write( handle, data, count );
      if( size_t(w) == count )
        return true;
      std::cerr << "serial::write error: " << strerror(errno) << std::endl;
      return false;
		}

		bool write( const char* data ) noexcept
		{ return write( data, strlen( data ) ); }

		bool write( const std::string& string ) noexcept
		{ return write( string.c_str(), string.size() ); }

		read_result read( void* data, size_t count, const std::chrono::milliseconds& timeout = infinite ) noexcept
		{
      const std::lock_guard<std::mutex> lock( mutex );

			if( timeout < 0s )
			{
				errno = ETIME;
				return read_result::failure( read_result::timeout );
			}

      last_access = clock::now();

			const auto x = serial_read( handle, data, &count, timeout == infinite ? NO_TIMEOUT : uint32_t( timeout.count() ) );

			switch( x )
			{
				case TIMEOUT:
					return read_result::failure( read_result::timeout );
				case 0:
					return read_result::failure( read_result::error );
				default:
					return read_result::success( count );
			}
		}

    read_result read_all( void* data, size_t count, const std::chrono::milliseconds& timeout = infinite ) noexcept
		{
      const std::lock_guard<std::mutex> lock( mutex );

			if( timeout.count() < 0 )
			{
				return read_result::failure( read_result::timeout );
			}

      last_access = clock::now();

			switch( serial_read_all( handle, data, count, timeout == infinite ? NO_TIMEOUT : uint32_t( timeout.count() ) ) )
			{
				case TIMEOUT:
					return read_result::failure( read_result::timeout );
				case 0:
					return read_result::failure( read_result::error );
				default:
					return read_result::success( count );
			}
		}

		template<typename type_t>
		inline file& write( const type_t& some )
		{
			if( not write( &some, sizeof(some) ) )
				throw std::runtime_error( strerror( errno ) );
			return *this;
		}

		template<typename type_t>
		typename std::enable_if<std::is_trivially_copyable<type_t>::value, file&>::type
		inline read( type_t& some, const std::chrono::milliseconds& timeout = infinite )
		{
			if( not read_all( &some, sizeof(some), timeout ) )
				throw std::runtime_error( strerror( errno ) );
			return *this;
		}

		template<typename type_t>
		typename std::enable_if<std::is_trivially_copyable<type_t>::value, type_t>::type
		inline read( const std::chrono::milliseconds& timeout = infinite )
		{
			type_t some;
			read( some, timeout );
			return some;
		}

		template<typename type_t>
		inline file& operator << ( const type_t& some )
		{
			return write( some );
		}

		template<typename type_t>
		inline file& operator >> ( type_t& some )
		{
			return read( some );
		}

    inline void flush()
    {
      serial_flush( handle );
    }

    clock::time_point get_last_access() const
    {
      return last_access;
    }

	private:

		file( const file& ) = delete;
		file& operator = ( const file& ) = delete;

		serial_t handle;
		std::mutex mutex;
    clock::time_point last_access;
	};
}

#endif // SERIAL_HPP
