#pragma once

#include "json5.hpp"

#include <functional>

namespace json5::detail {

//---------------------------------------------------------------------------------------------------------------------
inline void visit( const box_value &in, std::string_view pattern, std::function<void( box_value )> cb )
{
	if ( pattern.empty() )
	{
		cb( in );
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
				visit( kvp.second, tail, cb );
		}
		else if ( in.is_array() )
		{
			for ( auto v : array_view( in ) )
				visit( v, tail, cb );
		}
		else
			visit( in, std::string_view(), cb );
	}
	else if ( head == "**" )
	{
		if ( in.is_object() )
		{
			visit( in, tail, cb );

			for ( auto kvp : object_view( in ) )
				visit( kvp.second, pattern, cb );
		}
		else if ( in.is_array() )
		{
			for ( auto v : array_view( in ) )
				visit( v, pattern, cb );
		}
	}
	else
	{
		if ( in.is_object() )
		{
			for ( auto kvp : object_view( in ) )
			{
				if ( head == kvp.first )
					visit( kvp.second, tail, cb );
			}
		}
	}
}

} // namespace json5::detail
