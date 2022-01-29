#pragma once

#include "json5_base.hpp"

#if !defined(JSON5_DO_NOT_USE_STL)
#include <string>
#include <vector>
namespace json5 {
using string = std::string;
using string_view = std::string_view;
}
#else
#endif

namespace json5 {

/*

json5::value

*/
class value
{
public:
	// Construct null value
	value() noexcept;

	// Construct null value
	value( std::nullptr_t ) noexcept : _data( type_null ) { }

	// Construct boolean value
	value( bool val ) noexcept : _data( val ? type_true : type_false ) { }

	// Construct number value from int (will be converted to double)
	value( int val ) noexcept : _double( val ) { }

	// Construct number value from double
	value( double val ) noexcept : _double( val ) { }

	// Construct string value from null-terminated string
	value( const char *val ) noexcept : value( value_type::string, val ) { }

	// Return value type
	value_type type() const noexcept;

	// Checks, if value is null
	bool is_null() const noexcept { return _data == type_null; }

	// Checks, if value stores boolean. Use 'get_bool' for reading.
	bool is_boolean() const noexcept { return _data == type_true || _data == type_false; }

	// Checks, if value stores number. Use 'get_number' or 'try_get_number' for reading.
	bool is_number() const noexcept { return ( _data & mask_nanbits ) != mask_nanbits; }

	// Checks, if value stores string. Use 'get_c_str' for reading.
	bool is_string() const noexcept { return ( _data & mask_type ) == type_string; }

	// Checks, if value stores JSON object. Use 'object_view' wrapper
	// to iterate over key-value pairs (properties).
	bool is_object() const noexcept { return ( _data & mask_type ) == type_object; }

	// Checks, if value stores JSON array. Use 'array_view' wrapper
	// to iterate over array elements.
	bool is_array() const noexcept { return ( _data & mask_type ) == type_array; }

	// Check, if this is a root document value and can be safely casted to json5::document
	bool is_document() const noexcept { return ( _data & mask_is_document ) == mask_is_document; }

	// Get stored bool. Returns 'defaultValue', if this value is not a boolean.
	bool get_bool( bool defaultValue = false ) const noexcept;

	// Get stored string. Returns 'defaultValue', if this value is not a string.
	const char *get_c_str( const char *defaultValue = "" ) const noexcept;

	// Get stored number as type 'T'. Returns 'defaultValue', if this value is not a number.
	template <typename T>
	T get_number( T defaultValue = 0 ) const noexcept
	{
		return is_number() ? T( _double ) : defaultValue;
	}

	// Try to get stored number as type 'T'. Returns false, if this value is not a number.
	template <typename T>
	bool try_get_number( T &out ) const noexcept
	{
		if ( !is_number() )
			return false;

		out = T( _double );
		return true;
	}

	// Equality test against another value. Note that this might be a very expensive operation
	// for large nested JSON objects!
	bool operator==( const value &other ) const noexcept;

	// Non-equality test
	bool operator!=( const value &other ) const noexcept { return !( ( *this ) == other ); }

	// Use value as JSON object and get property value under 'key'. If this value
	// is not an object or 'key' is not found, null value is always returned.
	value operator[]( string_view key ) const noexcept;

	// Use value as JSON array and get item at 'index'. If this value is not
	// an array or index is out of bounds, null value is returned.
	value operator[]( size_t index ) const noexcept;

	// Get value payload (lower 48bits of _data) converted to type 'T'
	template <typename T> T payload() const noexcept { return ( T )( _data & mask_payload ); }

	// Get location in the original file (line, column & byte offset)
	location loc() const noexcept { return _loc; }

	template <typename T>
	[[deprecated( "Use get_number instead" )]]
	T get( T defaultValue = 0 ) const noexcept { return get_number<T>( defaultValue ); }

	template <typename T>
	[[deprecated( "Use try_get_number" )]]
	bool try_get( T &out ) const noexcept { return try_get_number( out ); }

protected:
	value( value_type t, uint64_t data );
	value( value_type t, const void *data ) : value( t, reinterpret_cast<uint64_t>( data ) ) { }

	void relink( const class document *prevDoc, class document &doc ) noexcept;

	// NaN-boxed data
	union
	{
		double _double;
		uint64_t _data;
	};

	// Location in source file
	location _loc = { };

	static constexpr uint64_t mask_nanbits     = 0xFFF0000000000000ull;
	static constexpr uint64_t mask_type        = 0xFFF7000000000000ull;
	static constexpr uint64_t mask_is_document = 0x0008000000000000ull;
	static constexpr uint64_t mask_payload     = 0x0000FFFFFFFFFFFFull;
	static constexpr uint64_t type_false       = 0xFFF1000000000000ull;
	static constexpr uint64_t type_true        = 0xFFF2000000000000ull;
	static constexpr uint64_t type_string      = 0xFFF3000000000000ull;
	static constexpr uint64_t type_string_off  = 0xFFF4000000000000ull;
	static constexpr uint64_t type_array       = 0xFFF5000000000000ull;
	static constexpr uint64_t type_object      = 0xFFF6000000000000ull;
	static constexpr uint64_t type_null        = 0xFFF7000000000000ull;

	// Stores lower 48bits of uint64 as payload
	void payload( uint64_t p ) noexcept { _data = ( _data & ~mask_payload ) | p; }

	// Stores lower 48bits of a pointer as payload
	void payload( const void *p ) noexcept { payload( reinterpret_cast<uint64_t>( p ) ); }

	friend document;
	friend builder;
	friend parser;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*

json5::document

*/
class document final : public value
{
public:
	// Construct empty document
	document() : value() { _data = value::type_null | value::mask_is_document; }

	// Construct a document copy
	document( const document &copy ) { assign_copy( copy ); }

	// Construct a document from r-value
	document( document &&rValue ) noexcept { assign_rvalue( std::forward<document>( rValue ) ); }

	// Copy data from another document (does a deep copy)
	document &operator=( const document &copy ) { assign_copy( copy ); return *this; }

	// Assign data from r-value (does a swap)
	document &operator=( document &&rValue ) noexcept { assign_rvalue( std::move( rValue ) ); return *this; }

private:
	detail::string_offset alloc_string( const char *str, size_t length = size_t( -1 ) );

	void reset() noexcept;

	void assign_copy( const document &copy );
	void assign_rvalue( document &&rValue ) noexcept;
	void assign_root( value root ) noexcept;

	string _strings;
	std::vector<value> _values;

	friend value;
	friend builder;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*

json5::object_view

*/
class object_view final
{
public:
	// Construct an empty object view
	object_view() noexcept = default;

	// Construct object view over a value. If the provided value does not reference a JSON object,
	// this object_view will be created empty (and invalid)
	object_view( const value &v ) noexcept
		: _pair( v.is_object() ? ( v.payload<const value*>() + 1 ) : nullptr )
		, _count( _pair ? ( _pair[-1].get_number<size_t>() / 2 ) : 0 )
	{ }

	// Checks, if object view was constructed from valid value
	bool is_valid() const noexcept { return _pair != nullptr; }

	// Source JSON value (first key value in first key-value pair)
	const value *source() const noexcept { return _pair; }

	// Location of the source value, returns invalid location for invalid view
	location loc() const noexcept { return _pair ? _pair->loc() : location(); }

	struct key_value_pair
	{
		string_view first = string_view();
		value second = value();
	};

	class iterator final
	{
	public:
		iterator( const value *p = nullptr ) noexcept : _pair( p ) { }
		bool operator!=( const iterator &other ) const noexcept { return _pair != other._pair; }
		bool operator==( const iterator &other ) const noexcept { return _pair == other._pair; }
		iterator &operator++() noexcept { _pair += 2; return *this; }
		key_value_pair operator*() const noexcept { return key_value_pair( _pair[0].get_c_str(), _pair[1] ); }

	private:
		const value *_pair = nullptr;
	};

	// Get an iterator to the beginning of the object (first key-value pair)
	iterator begin() const noexcept { return iterator( _pair ); }

	// Get an iterator to the end of the object (past the last key-value pair)
	iterator end() const noexcept { return iterator( _pair + _count * 2 ); }

	// Find property value with 'key'. Returns end iterator, when not found.
	iterator find( string_view key ) const noexcept;

	// Get number of key-value pairs
	size_t size() const noexcept { return _count; }

	// True, when object is empty
	bool empty() const noexcept { return size() == 0; }

	// Returns value associated with specified key or invalid value, when key is not found
	value operator[]( string_view key ) const noexcept;

	// Returns key-value pair at specified index
	key_value_pair operator[]( size_t index ) const noexcept;

	bool operator==( const object_view &other ) const noexcept;
	bool operator!=( const object_view &other ) const noexcept { return !( ( *this ) == other ); }

private:
	const value *_pair = nullptr;
	size_t _count = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*

json5::array_view

*/
class array_view final
{
public:
	// Construct an empty array view
	array_view() noexcept = default;

	// Construct array view over a value. If the provided value does not reference a JSON array,
	// this array_view will be created empty (and invalid)
	array_view( const value &v ) noexcept
		: _value( v.is_array() ? ( v.payload<const value*>() + 1 ) : nullptr )
		, _count( _value ? _value[-1].get_number<size_t>() : 0 )
	{ }

	// Checks, if array view was constructed from valid value
	bool is_valid() const noexcept { return _value != nullptr; }

	// Source JSON value (first array item)
	const value *source() const noexcept { return _value; }

	// Location of the source value, returns invalid location for invalid view
	location loc() const noexcept { return _value ? _value->loc() : location(); }

	using iterator = const value*;

	iterator begin() const noexcept { return _value; }
	iterator end() const noexcept { return _value + _count; }
	size_t size() const noexcept { return _count; }
	bool empty() const noexcept { return _count == 0; }
	value operator[]( size_t index ) const noexcept;

	bool operator==( const array_view &other ) const noexcept;
	bool operator!=( const array_view &other ) const noexcept { return !( ( *this ) == other ); }

private:
	const value *_value = nullptr;
	size_t _count = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline value::value() noexcept
	: _data( 0 )
{

}

//---------------------------------------------------------------------------------------------------------------------
inline value::value( value_type t, uint64_t data )
{
	if ( t == value_type::object )
		_data = type_object | data;
	else if ( t == value_type::array )
		_data = type_array | data;
	else if ( t == value_type::string )
		_data = type_string | data;
	else
		_data = data;
}

//---------------------------------------------------------------------------------------------------------------------
inline value_type value::type() const noexcept
{
	if ( ( _data & mask_nanbits ) != mask_nanbits )
		return value_type::number;

	if ( ( _data & mask_type ) == type_object )
		return value_type::object;
	else if ( ( _data & mask_type ) == type_array )
		return value_type::array;
	else if ( ( _data & mask_type ) == type_string )
		return value_type::string;
	if ( _data == type_true || _data == type_false )
		return value_type::boolean;

	return value_type::null;
}

//---------------------------------------------------------------------------------------------------------------------
inline bool value::get_bool( bool defaultValue ) const noexcept
{
	if ( _data == type_true )
		return true;
	else if ( _data == type_false )
		return false;

	return defaultValue;
}

//---------------------------------------------------------------------------------------------------------------------
inline const char *value::get_c_str( const char *defaultValue ) const noexcept
{
	return is_string() ? payload<const char *>() : defaultValue;
}

//---------------------------------------------------------------------------------------------------------------------
inline bool value::operator==( const value &other ) const noexcept
{
	if ( auto t = type(); t == other.type() )
	{
		if ( t == value_type::null )
			return true;
		else if ( t == value_type::boolean )
			return _data == other._data;
		else if ( t == value_type::number )
			return _double == other._double;
		else if ( t == value_type::string )
			return string_view( payload<const char *>() ) == string_view( other.payload<const char *>() );
		else if ( t == value_type::array )
			return array_view( *this ) == array_view( other );
		else if ( t == value_type::object )
			return object_view( *this ) == object_view( other );
	}

	return false;
}

//---------------------------------------------------------------------------------------------------------------------
inline value value::operator[]( string_view key ) const noexcept
{
	if ( !is_object() )
		return value();

	object_view ov( *this );
	return ov[key];
}

//---------------------------------------------------------------------------------------------------------------------
inline value value::operator[]( size_t index ) const noexcept
{
	if ( !is_array() )
		return value();

	array_view av( *this );
	return av[index];
}

//---------------------------------------------------------------------------------------------------------------------
inline void value::relink( const class document *prevDoc, class document &doc ) noexcept
{
	if ( ( _data & mask_type ) == type_string_off )
	{
		payload( doc._strings.data() + payload<uint64_t>() );

		_data &= ~mask_type;
		_data |= type_string;
	}
	else if ( ( _data & mask_type ) == type_string )
	{
		if ( prevDoc )
		{
			auto prevOffset = payload<const char *>() - prevDoc->_strings.data();
			payload( doc._strings.data() + prevOffset );
		}
		else
		{
			if ( auto *str = get_c_str(); str < doc._strings.data() || str >= doc._strings.data() + doc._strings.size() )
			{
				auto newOffset = doc.alloc_string( str );
				payload( doc._strings.data() + newOffset );
			}
		}
	}
	else if ( is_object() || is_array() )
	{
		if ( prevDoc )
			payload( payload<const value *>() - prevDoc->_values.data() );

		payload( doc._values.data() + payload<uint64_t>() );
	}
}

//---------------------------------------------------------------------------------------------------------------------
inline detail::string_offset document::alloc_string( const char *str, size_t length )
{
	if ( length == size_t( -1 ) )
		length = str ? strlen( str ) : 0;

	if ( !str || !length )
		return 0;

	auto result = detail::string_offset( _strings.size() );
	_strings.append( str, length );
	_strings.push_back( 0 );
	return result;
}

//---------------------------------------------------------------------------------------------------------------------
inline void document::reset() noexcept
{
	_data = value::type_null | value::mask_is_document;
	_values.clear();
	_strings.clear();
	_strings.push_back( 0 );
}

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
	_data = root._data | mask_is_document;

	for ( auto &v : _values )
		v.relink( nullptr, *this );

	relink( nullptr, *this );
}

//---------------------------------------------------------------------------------------------------------------------
inline object_view::iterator object_view::find( string_view key ) const noexcept
{
	if ( !key.empty() )
	{
		for ( auto iter = begin(); iter != end(); ++iter )
			if ( key == ( *iter ).first )
				return iter;
	}

	return end();
}

//---------------------------------------------------------------------------------------------------------------------
inline value object_view::operator[]( string_view key ) const noexcept
{
	const auto iter = find( key );
	return ( iter != end() ) ? ( *iter ).second : value();
}

//---------------------------------------------------------------------------------------------------------------------
inline object_view::key_value_pair object_view::operator[]( size_t index ) const noexcept
{
	if ( index >= _count )
		return key_value_pair();

	return { _pair[index * 2].get_c_str(), _pair[index * 2 + 1] };
}

//---------------------------------------------------------------------------------------------------------------------
inline bool object_view::operator==( const object_view &other ) const noexcept
{
	if ( size() != other.size() )
		return false;

	if ( empty() )
		return true;

	for ( size_t i = 0, S = _count * 2; i < S; i += 2 )
	{
		if ( _pair[i] != other._pair[i] )
			return false;

		if ( _pair[i + 1] != other._pair[i + 1] )
			return false;
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
inline value array_view::operator[]( size_t index ) const noexcept
{
	return ( index < _count ) ? _value[index] : value();
}

//---------------------------------------------------------------------------------------------------------------------
inline bool array_view::operator==( const array_view &other ) const noexcept
{
	if ( size() != other.size() )
		return false;

	auto iter = begin();
	for ( const auto &v : other )
		if ( *iter++ != v )
			return false;

	return true;
}

} // namespace json5
