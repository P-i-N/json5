#pragma once

#include "json5.hpp"

#include <functional>

namespace json5::detail {

//---------------------------------------------------------------------------------------------------------------------
inline void filter( const value &in, std::string_view pattern, values_t &out )
{
	if ( pattern.empty() )
	{
		out.push_back( in );
		return;
	}

	std::string_view head = pattern;
	std::string_view tail = pattern;

	if ( auto slash = pattern.find( "/" ); slash != std::string_view::npos )
	{
		head = pattern.substr( 0, slash );
		tail = pattern.substr( slash + 1 );
	}
	else
		tail = std::string_view();

	if ( head == "*" )
	{
		if ( in.is_object() )
		{
			for ( auto kvp : object_view( in ) )
				filter( kvp.second, tail, out );
		}
		else if ( in.is_array() )
		{
			for ( auto v : array_view( in ) )
				filter( v, tail, out );
		}
		else
			filter( in, std::string_view(), out );
	}
	else if ( head == "**" )
	{
		if ( in.is_object() )
		{
			filter( in, tail, out );

			for ( auto kvp : object_view( in ) )
				filter( kvp.second, pattern, out );
		}
		else if ( in.is_array() )
		{
			for ( auto v : array_view( in ) )
				filter( v, pattern, out );
		}
	}
	else
	{
		if ( in.is_object() )
		{
			for ( auto kvp : object_view( in ) )
			{
				if ( head == kvp.first )
					filter( kvp.second, tail, out );
			}
		}
	}
}

} // namespace json5::detail
