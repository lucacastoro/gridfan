#include "libgridfan.h"

#include <iostream>

namespace grid {

	controller::controller( const char* filename )
		: file( filename, serial::configuration::make8N1( 4800 ) )
	{
		// TODO init and fill fans

		fans.emplace_back( file, 0 );
		fans.emplace_back( file, 1 );
		fans.emplace_back( file, 2 );
		fans.emplace_back( file, 3 );
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

	fan::fan( serial::file& f, size_t i )
		: file( f )
		, index( i )
	{}

	size_t fan::id() const
	{
		return index;
	}

	int fan::getSpeed() const
	{
		// TODO: ALL
		return 0;
	}

	int fan::getPercent() const
	{
		// TODO: ALL
		return 0;
	}

	void fan::setPercent( int pr ) const
	{
		// TODO: ALL
		std::cout << "speed for fan " << id() << " set to " << pr << "%" << std::endl;
	}
}
