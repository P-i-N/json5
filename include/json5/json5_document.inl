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

	const box_value &root() const noexcept { return _values[0]; }

	using values_t = std::vector<box_value>;

	values_t operator()( std::string_view pattern ) const noexcept;

private:
	void assign_copy( const document &copy );
	void assign_rvalue( document &&rValue ) noexcept;

	std::string _strings;
	values_t _values = { box_value() };

	friend box_value;
	friend class builder;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
void document::assign_copy( const document &copy )
{
	_strings = copy._strings;
	_values = copy._values;

	for ( auto &v : _values )
		v.relink( &copy, *this );
}

//---------------------------------------------------------------------------------------------------------------------
void document::assign_rvalue( document &&rValue ) noexcept
{
	_strings = std::move( rValue._strings );
	_values = std::move( rValue._values );

	for ( auto &v : _values )
		v.relink( &rValue, *this );

	for ( auto &v : rValue._values )
		v.relink( this, rValue );
}

//---------------------------------------------------------------------------------------------------------------------
std::vector<box_value> document::operator()( std::string_view pattern ) const noexcept
{
	std::vector<box_value> result;
	detail::visit( root(), pattern, [&result]( box_value v ) { result.push_back( v ); } );
	return result;
}

} // namepsace json5
