#pragma once

#include "json5.hpp"

#include <iomanip>

namespace json5::detail {

//---------------------------------------------------------------------------------------------------------------------
struct string_style
{
	char quotes = '"';
	bool escape_unicode = false;
};

//---------------------------------------------------------------------------------------------------------------------
inline void to_stream( std::ostream &os, const char *str, const string_style &style )
{
	if ( style.quotes )
		os << style.quotes;

	while ( *str )
	{
		bool advance = true;

		if ( str[0] == '\n' )
			os << "\\n";
		else if ( str[0] == '\r' )
			os << "\\r";
		else if ( str[0] == '\t' )
			os << "\\t";
		else if ( str[0] == '"' && style.quotes == '"' )
			os << "\\\"";
		else if ( str[0] == '\'' && style.quotes == '\'' )
			os << "\\'";
		else if ( str[0] == '\\' )
			os << "\\\\";
		else if ( static_cast<uint8_t>( str[0] ) >= 128 && style.escape_unicode )
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
				os << "?";

			advance = false;
		}
		else
			os << *str;

		if ( advance )
			++str;
	}

	if ( style.quotes )
		os << style.quotes;
}

} // namespace json5::detail
