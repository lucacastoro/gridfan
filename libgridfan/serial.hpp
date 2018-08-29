#ifndef SERIAL_HPP
#define SERIAL_HPP

#include <utility>
#include <cmath>
#include <cstring>
#include <string>
#include <cassert>
#include <stdexcept>

#include "serial.h"

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

		static bool almost( float a, float b, float delta = 0.01 )
		{
			return std::abs( a - b ) < delta;
		}

		serial_config_t config;
	};

	typedef configuration config;

	class file
	{
	public:
		file() noexcept
			: handle( INVALID_SERIAL )
		{}

		file( const char* filename, const configuration& config ) noexcept
			: handle( serial_open( filename, *config ) )
		{}

		file( file&& other ) noexcept
		{ (*this) = std::move( other ); }

		file& operator = ( file&& other ) noexcept
		{
			if( this == &other ) return *this;
			if( *this ) serial_close( handle );
			this->handle = other.handle;
			other.handle = INVALID_SERIAL;
			return *this;
		}

		virtual ~file()
		{ if( *this ) serial_close( handle ); }

		explicit operator bool () const noexcept
		{ return INVALID_SERIAL != handle; }

		bool write( const void* data, size_t count ) noexcept
		{ return 0 != serial_write( handle, data, count ); }

		bool write( const char* data ) noexcept
		{ return write( data, strlen( data ) ); }

		bool write( const std::string& string ) noexcept
		{ return write( string.c_str(), string.size() ); }

		size_t read( void* data, size_t count ) noexcept
		{
			uint32_t len = count;
			if( 0 == serial_read( handle, data, &len ) )
				return 0;
			return len;
		}

		bool read_all( void* data, size_t count ) noexcept
		{ return serial_read_all( handle, data, count ); }

		template<typename type_t>
		file& operator << ( const type_t& some )
		{
			if( not write( &some, sizeof(some) ) ) throw std::runtime_error( strerror( errno ) );
			return *this;
		}

		template<typename type_t>
		file& operator >> ( type_t& some )
		{
			if( not read_all( &some, sizeof(some) ) ) throw std::runtime_error( strerror( errno ) );
			return *this;
		}

	private:

		file( const file& ) = delete;
		file& operator = ( const file& ) = delete;

		serial_t handle;
	};
}

#endif // SERIAL_HPP
