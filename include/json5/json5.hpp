#pragma once

#include "json5_base.hpp"

#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace json5 {

// Writes json5::document into stream
void to_stream( std::ostream &os, const document &doc, const writer_params &wp = writer_params() );

// Converts json5::document to string
void to_string( std::string &str, const document &doc, const writer_params &wp = writer_params() );

// Returns json5::document converted to string
std::string to_string( const document &doc, const writer_params &wp = writer_params() );

// Write json5::document into file, returns 'true' on success
bool to_file( const std::string &fileName, const document &doc, const writer_params &wp = writer_params() );

// Parse json5::document from stream
error from_stream( std::istream &is, document &doc );

// Parse json5::document from string
error from_string( const std::string &str, document &doc );

// Parse json5::document from file
error from_file( const std::string &fileName, document &doc );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*

json5::value

*/
class value
{
public:
	// Construct null value
	value() noexcept = default;

	// Construct null value
	value( std::nullptr_t ) noexcept : _data( type_null ) { }

	// Construct boolean value
	value( bool val ) noexcept : _data( val ? type_true : type_false ) { }

	// Construct number value from int (will be converted to double)
	value( int val ) noexcept : _double( static_cast<double>( val ) ) { }

	// Construct number value from float (will be converted to double)
	value( float val ) noexcept : _double( static_cast<double>( val ) ) { }

	// Construct number value from double
	value( double val ) noexcept : _double( val ) { }

	// Return value type
	value_type type() const noexcept;

	// Checks, if value is null
	bool is_null() const noexcept { return _data == type_null; }

	// Checks, if value stores boolean. Use 'get_bool' for reading.
	bool is_boolean() const noexcept { return _data == type_true || _data == type_false; }

	// Checks, if value stores number. Use 'get' or 'try_get' for reading.
	bool is_number() const noexcept { return ( _data & mask_nanbits ) != mask_nanbits; }

	// Checks, if value stores string. Use 'get_c_str' for reading.
	bool is_string() const noexcept { return ( _data & mask_type ) == type_string; }

	// Checks, if value stores JSON object. Use 'object_view' wrapper
	// to iterate over key-value pairs (properties).
	bool is_object() const noexcept { return ( _data & mask_type ) == type_object; }

	// Checks, if value stores JSON array. Use 'array_view' wrapper
	// to iterate over array elements.
	bool is_array() const noexcept { return ( _data & mask_type ) == type_array; }

	// Get stored bool. Returns 'defaultValue', if this value is not a boolean.
	bool get_bool( bool defaultValue = false ) const noexcept;

	// Get stored string. Returns 'defaultValue', if this value is not a string.
	const char *get_c_str( const char *defaultValue = "" ) const noexcept;

	// Get stored number as type 'T'. Returns 'defaultValue', if this value is not a number.
	template <typename T>
	T get( T defaultValue = 0 ) const noexcept
	{
		return is_number() ? static_cast<T>( _double ) : defaultValue;
	}

	// Try to get stored number as type 'T'. Returns false, if this value is not a number
	// and sets 'out' refernce to 'defaultValue'.
	template <typename T>
	bool try_get( T &out, T defaultValue = 0 ) const noexcept
	{
		if ( !is_number() )
		{
			out = defaultValue;
			return false;
		}

		out = static_cast<T>( _double );
		return true;
	}

	// Equality test against another value. Note that this might be a very expensive operation
	// for large nested JSON objects!
	bool operator==( const value &other ) const noexcept;

	// Non-equality test
	bool operator!=( const value &other ) const noexcept { return !( ( *this ) == other ); }

	value operator[]( std::string_view key ) const noexcept;

	value operator[]( size_t index ) const noexcept;

	// Returns vector of values filtered with specified pattern (see README.md or json5_filter.inl)
	std::vector<value> operator()( std::string_view pattern ) const noexcept;

	// Get value payload (lower 48bits of _data) converted to type 'T'
	template <typename T> T payload() const noexcept { return ( T )( _data & mask_payload ); }

private:
	value( value_type t, uint64_t data );
	value( value_type t, const void *data ) : value( t, reinterpret_cast<uint64_t>( data ) ) { }

	void relink( const class document *prevDoc, const class document &doc ) noexcept;

	// NaN-boxed data
	union
	{
		double _double;
		uint64_t _data;
	};

	static constexpr uint64_t mask_nanbits = 0xFFF0000000000000ull;
	static constexpr uint64_t mask_type    = 0xFFFF000000000000ull;
	static constexpr uint64_t mask_payload = 0x0000FFFFFFFFFFFFull;
	static constexpr uint64_t type_null    = 0xFFFC000000000000ull;
	static constexpr uint64_t type_false   = 0xFFF1000000000000ull;
	static constexpr uint64_t type_true    = 0xFFF3000000000000ull;
	static constexpr uint64_t type_string  = 0xFFF2000000000000ull;
	static constexpr uint64_t type_array   = 0xFFF4000000000000ull;
	static constexpr uint64_t type_object  = 0xFFF6000000000000ull;

	// Stores lower 48bits of uint64 as payload
	void payload( uint64_t p ) noexcept { _data = ( _data & ~mask_payload ) | p; }

	// Stores lower 48bits of a pointer as payload
	void payload( const void *p ) noexcept { payload( reinterpret_cast<uint64_t>( p ) ); }

	friend class document;
	friend class builder;
};

} // namespace json5

#include "json5_views.inl"
#include "json5_filter.inl"
#include "json5_document.inl"
#include "json5_builder.inl"
#include "json5_reader.inl"
#include "json5_output.inl"
#include "json5_reflect.inl"

namespace json5 {

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
		_data = type_null;
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
	return is_string() ? reinterpret_cast<const char *>( _data & mask_payload ) : defaultValue;
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
			return !strcmp(
			           reinterpret_cast<const char *>( _data & mask_payload ),
			           reinterpret_cast<const char *>( other._data & mask_payload ) );
		else if ( t == value_type::array )
			return array_view( *this ) == array_view( other );
		else if ( t == value_type::object )
			return object_view( *this ) == object_view( other );
	}

	return false;
}

//---------------------------------------------------------------------------------------------------------------------
inline value value::operator[]( std::string_view key ) const noexcept
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
inline std::vector<value> value::operator()( std::string_view pattern ) const noexcept
{
	std::vector<value> result;
	detail::filter( *this, pattern, result );
	return result;
}

//---------------------------------------------------------------------------------------------------------------------
inline void value::relink( const class document *prevDoc, const class document &doc ) noexcept
{
	if ( is_string() )
	{
		if ( prevDoc )
			payload( payload<const char *>() - prevDoc->_strings.data() );

		payload( doc._strings.data() + payload<uint64_t>() );
	}
	else if ( is_object() || is_array() )
	{
		if ( prevDoc )
			payload( payload<const value *>() - prevDoc->_values.data() );

		payload( doc._values.data() + payload<uint64_t>() );
	}
}

//---------------------------------------------------------------------------------------------------------------------
inline void to_stream( std::ostream &os, const error &err )
{
	os << error::type_string[err.type] << " at " << err.line << ":" << err.column;
}

//---------------------------------------------------------------------------------------------------------------------
inline std::string to_string( const error &err )
{
	std::ostringstream os;
	to_stream( os, err );
	return os.str();
}

//---------------------------------------------------------------------------------------------------------------------
inline void to_stream( std::ostream &os, const document &doc, const writer_params &wp )
{
	detail::to_stream( os, doc.root(), wp, 0 );
}

//---------------------------------------------------------------------------------------------------------------------
inline void to_string( std::string &str, const document &doc, const writer_params &wp )
{
	std::ostringstream os;
	to_stream( os, doc, wp );
	str = os.str();
}

//---------------------------------------------------------------------------------------------------------------------
inline std::string to_string( const document &doc, const writer_params &wp )
{
	std::string result;
	to_string( result, doc, wp );
	return result;
}

//---------------------------------------------------------------------------------------------------------------------
inline bool to_file( const std::string &fileName, const document &doc, const writer_params &wp )
{
	std::ofstream ofs( fileName );
	to_stream( ofs, doc, wp );
	return true;
}

//---------------------------------------------------------------------------------------------------------------------
inline error from_stream( std::istream &is, document &doc )
{
	detail::reader r{ doc, is };
	return r.parse();
}

//---------------------------------------------------------------------------------------------------------------------
inline error from_string( const std::string &str, document &doc )
{
	std::istringstream is( str );
	return from_stream( is, doc );
}

//---------------------------------------------------------------------------------------------------------------------
inline error from_file( const std::string &fileName, document &doc )
{
	std::ifstream ifs( fileName );
	return from_stream( ifs, doc );
}

} // namespace json5
