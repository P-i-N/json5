#pragma once

#include "json5.hpp"

namespace json5 {

class document final
{
public:
	document() = default;
	document( const document &copy ) { assign_copy( copy ); }
	document( document &&rValue ) noexcept { assign_rvalue( std::forward<document>( rValue ) ); }

	document &operator=( const document &copy ) { assign_copy( copy ); return *this; }
	document &operator=( document &&rValue ) noexcept { assign_rvalue( std::forward<document>( rValue ) ); return *this; }

	bool operator==( const document &other ) const noexcept { return root() == other.root(); }
	bool operator!=( const document &other ) const noexcept { return !( ( *this ) == other ); }

	const value &root() const noexcept { return _values[0]; }

	std::vector<value> operator()( std::string_view pattern ) const noexcept { return root()( pattern ); }

private:
	void assign_copy( const document &copy );
	void assign_rvalue( document &&rValue ) noexcept;

	std::string _strings;
	std::vector<value> _values = { value() };

	friend value;
	friend class builder;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline void document::assign_copy( const document &copy )
{
	_strings = copy._strings;
	_values = copy._values;

	for ( auto &v : _values )
		v.relink( &copy, *this );
}

//---------------------------------------------------------------------------------------------------------------------
inline void document::assign_rvalue( document &&rValue ) noexcept
{
	_strings = std::move( rValue._strings );
	_values = std::move( rValue._values );

	for ( auto &v : _values )
		v.relink( &rValue, *this );

	for ( auto &v : rValue._values )
		v.relink( this, rValue );
}

} // namepsace json5
