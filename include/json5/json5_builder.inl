#pragma once

#include "json5.hpp"

namespace json5 {

class builder
{
public:
	builder( document &doc ) : _doc( doc ) { }

	document &doc() noexcept { return _doc; }
	const document &doc() const noexcept { return _doc; }

	detail::string_offset string_buffer_offset() const noexcept;
	detail::string_offset string_buffer_add( std::string_view str );
	void string_buffer_add( char ch ) { _doc._strings.push_back( ch ); }

	box_value new_string( detail::string_offset stringOffset ) { return box_value( value_type::string, stringOffset ); }
	box_value new_string( std::string_view str ) { return new_string( string_buffer_add( str ) ); }

	void push_object();
	void push_array();
	box_value pop();

	builder &operator+=( box_value v );
	box_value &operator[]( detail::string_offset keyOffset );
	box_value &operator[]( std::string_view key ) { return ( *this )[string_buffer_add( key )]; }

protected:
	box_value &root() noexcept { return _doc._values[0]; }
	void reset() noexcept;

	document &_doc;
	std::vector<box_value> _stack;
	std::vector<box_value> _values;
	std::vector<size_t> _counts;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline detail::string_offset builder::string_buffer_offset() const noexcept
{
	return static_cast<detail::string_offset>( _doc._strings.size() );
}

//---------------------------------------------------------------------------------------------------------------------
inline detail::string_offset builder::string_buffer_add( std::string_view str )
{
	auto offset = string_buffer_offset();
	_doc._strings += str;
	_doc._strings.push_back( 0 );
	return offset;
}

//---------------------------------------------------------------------------------------------------------------------
inline void builder::push_object()
{
	auto v = box_value( value_type::object, 0ull );
	_stack.emplace_back( v );
	_counts.push_back( 0 );
}

//---------------------------------------------------------------------------------------------------------------------
inline void builder::push_array()
{
	auto v = box_value( value_type::array, 0ull );
	_stack.emplace_back( v );
	_counts.push_back( 0 );
}

//---------------------------------------------------------------------------------------------------------------------
inline box_value builder::pop()
{
	auto result = _stack.back();
	auto count = _counts.back();

	result._data |= _doc._values.size();

	_doc._values.push_back( box_value( static_cast<double>( count ) ) );

	auto startIndex = _values.size() - count;
	for ( size_t i = startIndex, S = _values.size(); i < S; ++i )
		_doc._values.push_back( _values[i] );

	_values.resize( _values.size() - count );

	if ( _stack.size() == 1 )
	{
		_doc._values[0] = result;

		for ( auto &v : _doc._values )
			v.relink( nullptr, _doc );

		result = _doc._values[0];
	}

	_stack.pop_back();
	_counts.pop_back();
	return result;
}

//---------------------------------------------------------------------------------------------------------------------
inline builder &builder::operator+=( box_value v )
{
	_values.push_back( v );
	_counts.back() += 1;
	return *this;
}

//---------------------------------------------------------------------------------------------------------------------
inline box_value &builder::operator[]( detail::string_offset keyOffset )
{
	_values.push_back( new_string( keyOffset ) );
	_counts.back() += 2;
	return _values.emplace_back();
}

//---------------------------------------------------------------------------------------------------------------------
inline void builder::reset() noexcept
{
	_doc._values = { box_value() };
	_doc._strings.clear();
	_doc._strings.push_back( 0 );
}

} // namespace json5
