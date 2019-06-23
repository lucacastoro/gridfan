#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stdlib.h>

#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN
	#ifdef UNICODE
		#undef UNICODE
	#endif
	#include <Windows.h>
	typedef HANDLE serial_t;
	#define INVALID_SERIAL 0
#else
	typedef int serial_t;
	#define INVALID_SERIAL -1
#endif

/* note that not every combination is supported by all platforms */

#define DATABITS_5 1
#define DATABITS_6 2
#define DATABITS_7 3
#define DATABITS_8 4
#define DATABITS_9 5

#define PARITY_NONE  1
#define PARITY_ODD   2
#define PARITY_EVEN  3
#define PARITY_MARK  4
#define PARITY_SPACE 5

#define STOPBIT_ONE      1
#define STOPBIT_ONE_HALF 2
#define STOPBIT_TWO      3

#define NO_TIMEOUT 0xFFFFFFFF
#define TIMEOUT NO_TIMEOUT

typedef struct serial_config_t
{
	uint32_t baudrate;
	uint32_t databits;
	uint32_t parity;
	uint32_t stopbits;
} serial_config_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief opens a file descriptor identified by "filename" setting it to the speed specified in "baudrate".
 * @param filenam: the serial file descriptor full name (eg. "/sys/ttyS22")
 * @param baudrate: the serial speed (eg. 9600)
 * @return a serial handle to use for consequential function calls or INVALID_SERIAL in case of error
*/
serial_t serial_open( const char* filename, const serial_config_t* settings );

/**
 * @brief closes the "serial" serial file
 * @param serial: the serial handle to close
 * @return nothing
*/
void serial_close( serial_t serial );

/**
 * @brief reads up to "buff_size" bytes from "serial" storing
 * them into "buffer" and writing the amount of bytes read into "buff_size"
 * @param serial: the serial handle to read
 * @param buffer: the buffer where put the read data
 * @param buff_size: the maximum amount of data to read, will contain the amount of data actually read
 * @return 1 if succesfull, 0 otherwise
*/
size_t serial_read( serial_t serial, void* buffer, size_t *buff_size, uint32_t timeout_ms );

/**
 * @brief reads exactly "buff_size" bytes from "serial" storing them into "buffer"
 * @param serial: the serial handle to read
 * @param buffer: the buffer where put the read data
 * @param buff_size: the amount of data to read
 * @return 1 if succesfull, 0 otherwise
*/
size_t serial_read_all( serial_t serial, void* buffer, size_t buff_size, uint32_t timeout_ms );

/**
 * @brief writes "buff_size" bytes to "serial" reading them from "buffer"
 * @param serial: the serial handle to read
 * @param buffer: the buffer containing the data to write
 * @param buff_size: the amount of bytes to write
 * @return 1 if succesfull, 0 otherwise
*/
ssize_t serial_write( serial_t serial, const void* buffer, size_t buff_size );

/**
 * @brief flushes any cached data to the serial
 * @param serial: the serial handle to flush
*/
void serial_flush( serial_t serial );

/**
 * @brief configures "settings" in the common 8-N-1 mode
 * @param baudrate: the expected baudrate
 * @param settings: a pointer to the settings structure to configure
 * @return 1 if succesfull, 0 otherwise
*/
uint32_t serial_8N1( uint32_t baudrate, serial_config_t* settings );

#ifdef __cplusplus
}
#endif

#endif /* SERIAL_H */
