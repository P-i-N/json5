#pragma once

#include "json5.hpp"

#include <iomanip>

namespace json5::detail {

//---------------------------------------------------------------------------------------------------------------------
inline void to_stream( std::ostream &os, const char *str, char quotes, bool escapeUnicode )
{
	if ( quotes )
		os << quotes;

	while ( *str )
	{
		bool advance = true;

		if ( str[0] == '\n' )
			os << "\\n";
		else if ( str[0] == '\r' )
			os << "\\r";
		else if ( str[0] == '\t' )
			os << "\\t";
		else if ( str[0] == '"' && quotes == '"' )
			os << "\\\"";
		else if ( str[0] == '\'' && quotes == '\'' )
			os << "\\'";
		else if ( str[0] == '\\' )
			os << "\\\\";
		else if ( static_cast<uint8_t>( str[0] ) >= 128 && escapeUnicode )
		{
			uint32_t ch = 0;

			if ( ( *str & 0b1110'0000u ) == 0b1100'0000u )
			{
				ch |= ( ( *str++ ) & 0b0001'1111u ) << 6;
				ch |= ( ( *str++ ) & 0b0011'1111u );
			}
			else if ( ( *str & 0b1111'0000u ) == 0b1110'0000u )
			{
				ch |= ( ( *str++ ) & 0b0000'1111u ) << 12;
				ch |= ( ( *str++ ) & 0b0011'1111u ) << 6;
				ch |= ( ( *str++ ) & 0b0011'1111u );
			}
			else if ( ( *str & 0b1111'1000u ) == 0b1111'0000u )
			{
				ch |= ( ( *str++ ) & 0b0000'0111u ) << 18;
				ch |= ( ( *str++ ) & 0b0011'1111u ) << 12;
				ch |= ( ( *str++ ) & 0b0011'1111u ) << 6;
				ch |= ( ( *str++ ) & 0b0011'1111u );
			}
			else if ( ( *str & 0b1111'1100u ) == 0b1111'1000u )
			{
				ch |= ( ( *str++ ) & 0b0000'0011u ) << 24;
				ch |= ( ( *str++ ) & 0b0011'1111u ) << 18;
				ch |= ( ( *str++ ) & 0b0011'1111u ) << 12;
				ch |= ( ( *str++ ) & 0b0011'1111u ) << 6;
				ch |= ( ( *str++ ) & 0b0011'1111u );
			}
			else if ( ( *str & 0b1111'1110u ) == 0b1111'1100u )
			{
				ch |= ( ( *str++ ) & 0b0000'0001u ) << 30;
				ch |= ( ( *str++ ) & 0b0011'1111u ) << 24;
				ch |= ( ( *str++ ) & 0b0011'1111u ) << 18;
				ch |= ( ( *str++ ) & 0b0011'1111u ) << 12;
				ch |= ( ( *str++ ) & 0b0011'1111u ) << 6;
				ch |= ( ( *str++ ) & 0b0011'1111u );
			}

			if ( ch <= std::numeric_limits<uint16_t>::max() )
			{
				os << "\\u" << std::hex << std::setfill( '0' ) << std::setw( 4 ) << ch;
			}
			else
				os << "?"; // JSON can't encode Unicode chars > 65535 (emojis)

			advance = false;
		}
		else
			os << *str;

		if ( advance )
			++str;
	}

	if ( quotes )
		os << quotes;
}

//---------------------------------------------------------------------------------------------------------------------
inline void to_stream( std::ostream &os, const value &v, const writer_params &wp, int depth )
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
		os << "null";
	else if ( v.is_boolean() )
		os << ( v.get_bool() ? "true" : "false" );
	else if ( v.is_number() )
	{
		if ( double _, d = v.get<double>(); modf( d, &_ ) == 0.0 )
			os << v.get<int64_t>();
		else
			os << d;
	}
	else if ( v.is_string() )
	{
		detail::to_stream( os, v.get_c_str(), '"', wp.escape_unicode );
	}
	else if ( v.is_array() )
	{
		if ( auto av = json5::array_view( v ); !av.empty() )
		{
			os << "[" << eol;
			for ( size_t i = 0, S = av.size(); i < S; ++i )
			{
				for ( int i = 0; i <= depth; ++i ) os << wp.indentation;
				to_stream( os, av[i], wp, depth + 1 );
				if ( i < S - 1 ) os << ",";
				os << eol;
			}

			for ( int i = 0; i < depth; ++i ) os << wp.indentation;
			os << "]";
		}
		else
			os << "[]";
	}
	else if ( v.is_object() )
	{
		if ( auto ov = json5::object_view( v ); !ov.empty() )
		{
			os << "{" << eol;
			size_t count = ov.size();
			for ( auto kvp : ov )
			{
				for ( int i = 0; i <= depth; ++i ) os << wp.indentation;

				if ( wp.json_compatible )
					os << "\"" << kvp.first << "\"" << kvSeparator;
				else
					os << kvp.first << kvSeparator;

				to_stream( os, kvp.second, wp, depth + 1 );
				if ( --count ) os << ",";
				os << eol;
			}

			for ( int i = 0; i < depth; ++i ) os << wp.indentation;
			os << "}";
		}
		else
			os << "{}";
	}

	if ( !depth )
		os << eol;
}

} // namespace json5::detail
