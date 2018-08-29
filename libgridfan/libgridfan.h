#ifndef LIBFANGRID_H
#define LIBFANGRID_H

#include "util/serial.hpp"
#include <vector>

namespace grid
{
	class fan
	{
	public:
		fan( serial::file& f, size_t index );
		size_t id() const;

		int getSpeed() const;
		int getPercent() const;
		void setPercent( int ) const;

	private:
		serial::file& file;
		size_t index;
	};

	class controller
	{
	public:
		controller( const char* filename = "/dev/ttyACM0" );
		const char* test();

		typedef std::vector<fan>::iterator iterator;
		typedef std::vector<fan>::const_iterator const_iterator;

		size_t size() const;
		bool empty() const;

		fan& operator [] ( size_t index );
		const fan& operator [] ( size_t index ) const;

		iterator begin();
		iterator end();

		const_iterator begin() const;
		const_iterator end() const;

	private:
		serial::file file;
		std::vector<fan> fans;
	};
}

#endif // LIBFANGRID_H
