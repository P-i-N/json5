#pragma once

#include "json5.hpp"

namespace json5 {

class document final : public value
{
public:
	document() = default;
	document( const document &copy ) { assign_copy( copy ); }
	document( document &&rValue ) noexcept { assign_rvalue( std::forward<document>( rValue ) ); }

	document &operator=( const document &copy ) { assign_copy( copy ); return *this; }
	document &operator=( document &&rValue ) noexcept { assign_rvalue( std::forward<document>( rValue ) ); return *this; }

private:
	void assign_copy( const document &copy );
	void assign_rvalue( document &&rValue ) noexcept;
	void assign_root( value root ) noexcept;

	std::string _strings;
	std::vector<value> _values;

	friend value;
	friend builder;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline void document::assign_copy( const document &copy )
{
	_data = copy._data;
	_strings = copy._strings;
	_values = copy._values;

	for ( auto &v : _values )
		v.relink( &copy, *this );

	relink( &copy, *this );
}

//---------------------------------------------------------------------------------------------------------------------
inline void document::assign_rvalue( document &&rValue ) noexcept
{
	_data = std::move( rValue._data );
	_strings = std::move( rValue._strings );
	_values = std::move( rValue._values );

	for ( auto &v : _values )
		v.relink( &rValue, *this );

	for ( auto &v : rValue._values )
		v.relink( this, rValue );
}

//---------------------------------------------------------------------------------------------------------------------
inline void document::assign_root( value root ) noexcept
{
	_data = root._data;

	for ( auto &v : _values )
		v.relink( nullptr, *this );

	relink( nullptr, *this );
}

} // namepsace json5
