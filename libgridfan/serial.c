#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "serial.h"

#define READ_TIMEOUT_MS 100 // deve essere >= 100

#ifdef _WIN32

serial_t openSerial( const char* filename, unsigned baudrate )
{
	HANDLE hSerial = INVALID_SERIAL;
	DCB dcbSerialParams = { 0 };
	SerialMTIMEOUTS timeouts = { 0 };
	unsigned ok = 0;

	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	switch( settings->stopbits )
	{
		case STOPBIT_ONE:      dcbSerialParams.StopBits = ONESTOPBIT; break;
		case STOPBIT_ONE_HALF: dcbSerialParams.StopBits = ONE5STOPBITS; break;
		case STOPBIT_TWO:      dcbSerialParams.StopBits = TWOSTOPBITS; break;
		default: return INVALID_SERIAL;
	}

	switch( settings->databits )
	{
		case DATABITS_5: dcbSerialParams.ByteSize = 5; break;
		case DATABITS_6: dcbSerialParams.ByteSize = 6; break;
		case DATABITS_7: dcbSerialParams.ByteSize = 7; break;
		case DATABITS_8: dcbSerialParams.ByteSize = 8; break;
		case DATABITS_9: dcbSerialParams.ByteSize = 9; break;
		default: return INVALID_SERIAL;
	}

	switch( settings->parity )
	{
		case PARITY_NONE : dcbSerialParams.Parity = NOPARITY; break;
		case PARITY_ODD  : dcbSerialParams.Parity = ODDPARITY; break;
		case PARITY_EVEN : dcbSerialParams.Parity = EVENPARITY; break;
		case PARITY_MARK : dcbSerialParams.Parity = MARKPARITY; break;
		case PARITY_SPACE: dcbSerialParams.Parity = SPACEPARITY; break;
		default: return INVALID_SERIAL;
	}

	if( INVALID_HANDLE_VALUE != ( hSerial = CreateFileA(
		filename,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0 ) ) )
	{
		if( GetCommState( hSerial, &dcbSerialParams ) )
		{
			dcbSerialParams.BaudRate = settings->baudrate;

			if( SetCommState( hSerial, &dcbSerialParams ) )
			{
				timeouts.ReadIntervalTimeout = READ_TIMEOUT_MS;
				timeouts.ReadTotalTimeoutMultiplier = 0;				/* TODO: multiplier == 0 ?!? */
				timeouts.ReadTotalTimeoutConstant = READ_TIMEOUT_MS;
				timeouts.WriteTotalTimeoutMultiplier = 10;
				timeouts.WriteTotalTimeoutConstant = 50;

				if( SetCommTimeouts( hSerial, &timeouts ) )
				{
					ok = 1;
				}
			}
		}

		if( ! ok )
		{
			CloseHandle( hSerial );
			hSerial = INVALID_SERIAL;
		}
	}
	else
	{
		// if( GetLastError() == ERROR_FILE_NOT_FOUND ) ...
	}

	return hSerial;
}

void closeSerial( serial_t com )
{
	CloseHandle( com );
}

unsigned readSerial( serial_t com, void* buffer, unsigned* buff_size )
{
	if( ( com != INVALID_SERIAL ) &&
		( buffer != 0 ) &&
		( buff_size != 0 )
	)
	{
		unsigned read;

		if( 0 != ReadFile( com, (unsigned char*)buffer, *buff_size, &read, NULL ) )
		{
			*buff_size = read;
			return 1;
		}
	}

	return 0;
}

unsigned writeSerial( serial_t com, void* buffer, unsigned buff_size )
{
	DWORD wrote;

	return
	(
		( INVALID_SERIAL != com ) &&
		( 0 != buffer ) &&
		( 0 != buff_size ) &&
		( 0 != WriteFile( com, buffer, buff_size, &wrote, NULL ) ) &&
		( wrote == buff_size )
	);
}

#else // not _WIN32

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

serial_t serial_open(const char* filename, const serial_config_t* settings )
{
	int32_t serial    = INVALID_SERIAL;
	speed_t speed     = 0;
	tcflag_t databits = 0;
	tcflag_t stopbits = 0;
	tcflag_t parity   = 0;

	switch( settings->baudrate )
	{
		case 50:     speed = B50; break;
		case 75:     speed = B75; break;
		case 110:    speed = B110; break;
		case 134:    speed = B134; break;
		case 150:    speed = B150; break;
		case 200:    speed = B200; break;
		case 300:    speed = B300; break;
		case 600:    speed = B600; break;
		case 1200:   speed = B1200; break;
		case 1800:   speed = B1800; break;
		case 2400:   speed = B2400; break;
		case 4800:   speed = B4800; break;
		case 9600:   speed = B9600; break;
		case 19200:  speed = B19200; break;
		case 38400:  speed = B38400; break;
		case 57600:  speed = B57600; break;
		case 115200: speed = B115200; break;
		case 230400: speed = B230400; break;
		default: errno = EINVAL; return INVALID_SERIAL;
	};

	switch( settings->databits )
	{
		case DATABITS_5: databits = CS5; break;
		case DATABITS_6: databits = CS6; break;
		case DATABITS_7: databits = CS7; break;
		case DATABITS_8: databits = CS8; break;
		default: errno = EINVAL; return INVALID_SERIAL;
	}

	switch( settings->stopbits )
	{
		case STOPBIT_ONE: stopbits = 0; break;
		case STOPBIT_TWO: stopbits = CSTOPB; break;
		default: errno = EINVAL; return INVALID_SERIAL;
	}

	switch( settings->parity )
	{
		case PARITY_NONE: break;
		case PARITY_EVEN: parity = PARENB; break;
		case PARITY_ODD:  parity = PARENB | PARODD; break;
		default: errno = EINVAL; return INVALID_SERIAL;
	}

	serial = open( filename, O_RDWR | O_NOCTTY | O_NDELAY );

  if( -1 == serial )
  {
    return INVALID_SERIAL;
  }

  struct termios config;
  memset( &config, 0, sizeof(config) );
  config.c_iflag = 0;
  config.c_oflag = 0;
  config.c_cflag = databits | parity | stopbits | CREAD | CLOCAL ;
  config.c_lflag = 0;
  config.c_cc[VMIN] = 1;
  config.c_cc[VTIME] = READ_TIMEOUT_MS / 100;

  if( 0 != cfsetospeed( &config, speed ) ||
      0 != cfsetispeed( &config, speed ) ||
      0 != tcsetattr( serial, TCSANOW, &config ) )
  {
    close( serial );
    return INVALID_SERIAL;
  }

	return serial;
}

void serial_close( serial_t serial )
{
	if( INVALID_SERIAL != serial )
	{
		close( serial );
	}
}

size_t serial_read( serial_t serial, void* buffer, size_t* buff_size , uint32_t timeout_ms )
{
  if( ( INVALID_SERIAL == serial ) ||
      ( NULL == buffer ) ||
      ( NULL == buff_size ) ||
      ( 0 == *buff_size ) )
	{
    errno = EINVAL;
  }
  else
  {
    if( NO_TIMEOUT == timeout_ms )
    {
      const ssize_t r = read( serial, buffer, *buff_size );

      if( r > 0 )
      {
        *buff_size = (uint32_t)r;
        return 1;
      }
      else
      {
        if( EAGAIN == errno )
        {
          *buff_size = 0;
          return 1;
        }
      }
    }
    else
    {
      const clock_t now = clock();
      struct timeval tv;
      fd_set rset;
      tv.tv_sec = timeout_ms / 1000;
      tv.tv_usec = ( timeout_ms % 1000 ) * 1000;
      FD_ZERO(&rset);
      FD_SET(serial, &rset);

      const int x = select( serial + 1, &rset, NULL, NULL, &tv );

      switch( x )
      {
        case -1:
          if(EAGAIN == errno)
          {
            const uint32_t ms = (uint32_t)(clock() - now) * 1000 / CLOCKS_PER_SEC;
            return serial_read( serial, buffer, buff_size, timeout_ms - ms );
          }
          break;
        case 0:
          errno = ETIME;
          break;
        default:
          return serial_read( serial, buffer, buff_size, NO_TIMEOUT );
      }
    }
  }

	return 0;
}

size_t serial_write( serial_t serial, const void* buffer, size_t buff_size )
{
  if( ( INVALID_SERIAL == serial ) ||
      ( NULL == buffer ) ||
      ( 0 == buff_size ) ) {
    errno = EINVAL;
    return 0;
  }
  else
  {
    ssize_t tot = 0;

    while( tot != (ssize_t)buff_size )
    {
      const ssize_t w = write( serial, ((const uint8_t*)buffer) + tot, buff_size - (size_t)tot );

      if( w < 1 )
      {
        return 0;
      }

      tot += w;
    }
  }
  return 1;
}

#endif // segue codice multipiattaforma

size_t serial_read_all( serial_t serial, void* buffer, size_t buff_size , uint32_t timeout_ms )
{
  size_t tot = 0;
  size_t   r = 0;

	do
	{
		r = buff_size - tot;

		if( !( serial_read( serial, ((uint8_t*)buffer) + tot, &r, timeout_ms ) ) )
		{
			return 0;
		}
	}
	while( ( tot += r ) != buff_size );

	return 1;
}

uint32_t serial_8N1( uint32_t baudrate, serial_config_t* settings )
{
	if( NULL == settings ) return 0;

	settings->baudrate = baudrate;
	settings->databits = DATABITS_8;
	settings->parity = PARITY_NONE;
	settings->stopbits = STOPBIT_ONE;

	return 1;
}
