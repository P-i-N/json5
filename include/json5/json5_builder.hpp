#pragma once

#include "json5.hpp"

namespace json5 {

class builder
{
public:
	builder( document &doc ) : _doc( doc ) { }

	const document &doc() const noexcept { return _doc; }

	detail::value new_string( string_view str ) { return new_string( string_buffer_add( str ) ); }

	void push_object();
	void push_array();
	detail::value pop();

	template <typename... Args>
	builder &operator()( Args... values )
	{
		bool results[] = { add_item( values )... };
		return *this;
	}

	detail::value &operator[]( string_view key );

protected:
	void reset() noexcept;

	detail::string_offset string_buffer_offset() const noexcept;
	detail::string_offset string_buffer_add( string_view str );
	void string_buffer_add( char ch ) { _doc._strings.push_back( ch ); }
	void string_buffer_add_utf8( uint32_t ch );

	detail::value new_string( detail::string_offset stringOffset )
	{
		return { value_type::null, detail::value::type_string_off | stringOffset };
	}

	bool add_item( detail::value v );

	document &_doc;
	std::vector<detail::value> _stack;
	std::vector<detail::value> _values;
	std::vector<size_t> _counts;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline void builder::reset() noexcept
{
	_doc.reset();
	_stack.clear();
	_values.clear();
	_counts.clear();
}

//---------------------------------------------------------------------------------------------------------------------
inline bool builder::add_item( detail::value v )
{
	if ( _stack.empty() )
		return false;

	_values.push_back( v );
	_counts.back() += 1;
	return true;
}

//---------------------------------------------------------------------------------------------------------------------
inline detail::string_offset builder::string_buffer_offset() const noexcept
{
	return detail::string_offset( _doc._strings.size() );
}

//---------------------------------------------------------------------------------------------------------------------
inline detail::string_offset builder::string_buffer_add( string_view str )
{
	return _doc.alloc_string( str.data(), str.size() );
}

//---------------------------------------------------------------------------------------------------------------------
inline void builder::string_buffer_add_utf8( uint32_t ch )
{
	auto &s = _doc._strings;

	if ( 0 <= ch && ch <= 0x7f )
	{
		s.push_back( uint8_t(ch) );
	}
	else if ( 0x80 <= ch && ch <= 0x7ff )
	{
		s.push_back( uint8_t( 0xc0 | ( ch >> 6 ) ) );
		s.push_back( uint8_t( 0x80 | ( ch & 0x3f ) ) );
	}
	else if ( 0x800 <= ch && ch <= 0xffff )
	{
		s.push_back( uint8_t( 0xe0 | ( ch >> 12 ) ) );
		s.push_back( uint8_t( 0x80 | ( ( ch >> 6 ) & 0x3f ) ) );
		s.push_back( uint8_t( 0x80 | ( ch & 0x3f ) ) );
	}
	else if ( 0x10000 <= ch && ch <= 0x1fffff )
	{
		s.push_back( uint8_t( 0xf0 | ( ch >> 18 ) ) );
		s.push_back( uint8_t( 0x80 | ( ( ch >> 12 ) & 0x3f ) ) );
		s.push_back( uint8_t( 0x80 | ( ( ch >> 6 ) & 0x3f ) ) );
		s.push_back( uint8_t( 0x80 | ( ch & 0x3f ) ) );
	}
	else if ( 0x200000 <= ch && ch <= 0x3ffffff )
	{
		s.push_back( uint8_t( 0xf8 | ( ch >> 24 ) ) );
		s.push_back( uint8_t( 0x80 | ( ( ch >> 18 ) & 0x3f ) ) );
		s.push_back( uint8_t( 0x80 | ( ( ch >> 12 ) & 0x3f ) ) );
		s.push_back( uint8_t( 0x80 | ( ( ch >> 6 ) & 0x3f ) ) );
		s.push_back( uint8_t( 0x80 | ( ch & 0x3f ) ) );
	}
	else if ( 0x4000000 <= ch && ch <= 0x7fffffff )
	{
		s.push_back( uint8_t( 0xfc | ( ch >> 30 ) ) );
		s.push_back( uint8_t( 0x80 | ( ( ch >> 24 ) & 0x3f ) ) );
		s.push_back( uint8_t( 0x80 | ( ( ch >> 18 ) & 0x3f ) ) );
		s.push_back( uint8_t( 0x80 | ( ( ch >> 12 ) & 0x3f ) ) );
		s.push_back( uint8_t( 0x80 | ( ( ch >> 6 ) & 0x3f ) ) );
		s.push_back( uint8_t( 0x80 | ( ch & 0x3f ) ) );
	}
}

//---------------------------------------------------------------------------------------------------------------------
inline void builder::push_object()
{
	_stack.push_back( detail::value( value_type::object, nullptr ) );
	_counts.push_back( 0 );
}

//---------------------------------------------------------------------------------------------------------------------
inline void builder::push_array()
{
	_stack.push_back( detail::value( value_type::array, nullptr ) );
	_counts.push_back( 0 );
}

//---------------------------------------------------------------------------------------------------------------------
inline detail::value builder::pop()
{
	auto result = _stack.back();
	size_t count = _counts.back();

	result.payload( _doc._values.size() );

	_doc._values.emplace_back( double( count ) );

	auto startIndex = _values.size() - count;
	for ( size_t i = startIndex, S = _values.size(); i < S; ++i )
		_doc._values.push_back( _values[i] );

	_values.resize( _values.size() - count );

	_stack.pop_back();
	_counts.pop_back();

	if ( _stack.empty() )
	{
		_doc.assign_root( result );
		result = _doc;
	}

	return result;
}

//---------------------------------------------------------------------------------------------------------------------
inline detail::value &builder::operator[]( string_view key )
{
	add_item( new_string( key ) );
	_counts.back() += 1;
	return _values.emplace_back();
}

} // namespace json5
