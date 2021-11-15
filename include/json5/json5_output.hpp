#pragma once

#include "json5.hpp"

#include <inttypes.h>

namespace json5 {

// Converts json5::document to string
void to_string( std::string &str, const document &doc, const writer_params &wp = writer_params() );

// Returns json5::document converted to string
std::string to_string( const document &doc, const writer_params &wp = writer_params() );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline void to_string( std::string &str, const char *utf8Str, char quotes, bool escapeUnicode )
{
	if ( quotes )
		str += quotes;

	while ( *utf8Str )
	{
		bool advance = true;

		if ( utf8Str[0] == '\n' )
			str += "\\n";
		else if ( utf8Str[0] == '\r' )
			str += "\\r";
		else if ( utf8Str[0] == '\t' )
			str += "\\t";
		else if ( utf8Str[0] == '"' && quotes == '"' )
			str += "\\\"";
		else if ( utf8Str[0] == '\'' && quotes == '\'' )
			str += "\\'";
		else if ( utf8Str[0] == '\\' )
			str += "\\\\";
		else if ( uint8_t( utf8Str[0] ) >= 128 && escapeUnicode )
		{
			uint32_t ch = 0;

			if ( ( *utf8Str & 0b1110'0000u ) == 0b1100'0000u )
			{
				ch |= ( ( *utf8Str++ ) & 0b0001'1111u ) << 6;
				ch |= ( ( *utf8Str++ ) & 0b0011'1111u );
			}
			else if ( ( *utf8Str & 0b1111'0000u ) == 0b1110'0000u )
			{
				ch |= ( ( *utf8Str++ ) & 0b0000'1111u ) << 12;
				ch |= ( ( *utf8Str++ ) & 0b0011'1111u ) << 6;
				ch |= ( ( *utf8Str++ ) & 0b0011'1111u );
			}
			else if ( ( *utf8Str & 0b1111'1000u ) == 0b1111'0000u )
			{
				ch |= ( ( *utf8Str++ ) & 0b0000'0111u ) << 18;
				ch |= ( ( *utf8Str++ ) & 0b0011'1111u ) << 12;
				ch |= ( ( *utf8Str++ ) & 0b0011'1111u ) << 6;
				ch |= ( ( *utf8Str++ ) & 0b0011'1111u );
			}
			else if ( ( *utf8Str & 0b1111'1100u ) == 0b1111'1000u )
			{
				ch |= ( ( *utf8Str++ ) & 0b0000'0011u ) << 24;
				ch |= ( ( *utf8Str++ ) & 0b0011'1111u ) << 18;
				ch |= ( ( *utf8Str++ ) & 0b0011'1111u ) << 12;
				ch |= ( ( *utf8Str++ ) & 0b0011'1111u ) << 6;
				ch |= ( ( *utf8Str++ ) & 0b0011'1111u );
			}
			else if ( ( *utf8Str & 0b1111'1110u ) == 0b1111'1100u )
			{
				ch |= ( ( *utf8Str++ ) & 0b0000'0001u ) << 30;
				ch |= ( ( *utf8Str++ ) & 0b0011'1111u ) << 24;
				ch |= ( ( *utf8Str++ ) & 0b0011'1111u ) << 18;
				ch |= ( ( *utf8Str++ ) & 0b0011'1111u ) << 12;
				ch |= ( ( *utf8Str++ ) & 0b0011'1111u ) << 6;
				ch |= ( ( *utf8Str++ ) & 0b0011'1111u );
			}

			if ( ch <= std::numeric_limits<uint16_t>::max() )
			{
				JSON5_ASSERT( 0 ); // TODO

				//os << "\\u" << std::hex << std::setfill( '0' ) << std::setw( 4 ) << ch;
			}
			else
				str += "?"; // JSON can't encode Unicode chars > 65535 (emojis)

			advance = false;
		}
		else
			str += *utf8Str;

		if ( advance )
			++utf8Str;
	}

	if ( quotes )
		str += quotes;
}

//---------------------------------------------------------------------------------------------------------------------
inline void to_string( std::string &str, const value &v, const writer_params &wp, int depth )
{
	const char *kvSeparator = ": ";
	const char *eol = wp.eol;

	if ( wp.compact )
	{
		depth = -1;
		kvSeparator = ":";
		eol = "";
	}

	if ( v.is_null() )
		str += "null";
	else if ( v.is_boolean() )
		str += ( v.get_bool() ? "true" : "false" );
	else if ( v.is_number() )
	{
		char buff[64] = { };

		if ( double _, d = v.get_number( 0.0 ); modf( d, &_ ) == 0.0 ) // Omit trailing zeros
			sprintf( buff, "%" PRIi64, int64_t( d ) );
		else
			sprintf( buff, "%lf", d );

		str += buff;
	}
	else if ( v.is_string() )
	{
		to_string( str, v.get_c_str(), '"', wp.escape_unicode );
	}
	else if ( v.is_array() )
	{
		if ( auto av = json5::array_view( v ); !av.empty() )
		{
			bool compact = ( av.size() <= wp.compact_array_size );

			str += "[";

			if ( !compact )
				str += eol;

			for ( size_t i = 0, S = av.size(); i < S; ++i )
			{
				if ( compact )
					str += " ";
				else
					for ( int i = 0; i <= depth; ++i ) str += wp.indentation;

				to_string( str, av[i], wp, depth + 1 );
				if ( i < S - 1 ) str += ",";

				if ( !compact )
					str += eol;
			}

			if ( compact )
				str += " ]";
			else
			{
				for (int i = 0; i < depth; ++i) str += wp.indentation;
				str += "]";
			}
		}
		else
			str += "[]";
	}
	else if ( v.is_object() )
	{
		if ( auto ov = json5::object_view( v ); !ov.empty() )
		{
			bool compact = ( ov.size() <= wp.compact_object_size );

			str += "{";

			if ( !compact )
				str += eol;

			size_t count = ov.size();
			for ( const auto &kvp : ov )
			{
				if ( compact )
					str += " ";
				else
					for ( int i = 0; i <= depth; ++i ) str += wp.indentation;

				if ( wp.json_compatible )
				{
					str += "\"";
					str += kvp.first;
					str += "\"";
				}
				else
					str += kvp.first;

				str += kvSeparator;

				to_string( str, kvp.second, wp, depth + 1 );
				if ( --count ) str += ",";

				if ( !compact )
					str += eol;
			}

			if ( compact )
				str += " }";
			else
			{
				for (int i = 0; i < depth; ++i) str += wp.indentation;
				str += "}";
			}
		}
		else
			str += "{}";
	}

	if ( !depth )
		str += eol;
}

//---------------------------------------------------------------------------------------------------------------------
inline std::string to_string( const document &doc, const writer_params &wp )
{
	std::string result;
	to_string( result, doc, wp, 0 );
	return result;
}

//---------------------------------------------------------------------------------------------------------------------
inline std::string to_string( const error &err )
{
	JSON5_ASSERT( 0 ); // TODO

	/*
	std::ostringstream os;
	to_stream( os, err );
	return os.str();
	*/

	return "";
}

} // namespace json5
