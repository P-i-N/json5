#pragma once

#include "json5_input.hpp"
#include "json5_reflect.hpp"

#include <fstream>

namespace json5 {

// Write json5::document into file, returns 'true' on success
bool to_file( string_view fileName, const document &doc, const writer_params &wp = writer_params() );

// Serialize instance of type 'T' into file
template <typename T> bool to_file( string_view fileName, const T &in, const writer_params &wp = writer_params() );

// Parse json5::document from file
error from_file( string_view fileName, document &doc );

// Initialize instance of type 'T' from file
template <typename T> error from_file( string_view fileName, T &out );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline bool to_file( string_view fileName, const document &doc, const writer_params &wp )
{
	std::ofstream ofs( string( fileName ).c_str() );
	if ( !ofs.is_open() )
		return false;

	ofs << to_string( doc, wp );
	return true;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline bool to_file( string_view fileName, const T &in, const writer_params &wp )
{
	std::ofstream ofs( string( fileName ).c_str() );
	if ( !ofs.is_open() )
		return false;

	document doc;
	to_document( doc, in, wp );

	ofs << to_string( doc );
	return true;
}

//---------------------------------------------------------------------------------------------------------------------
inline error from_file( string_view fileName, document &doc )
{
	std::ifstream ifs( string( fileName ).c_str() );
	if ( !ifs.is_open() )
		return { error::could_not_open };

	auto str = string( std::istreambuf_iterator<char>( ifs ), std::istreambuf_iterator<char>() );
	return from_string( string_view( str ), doc );
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error from_file( string_view fileName, T &out )
{
	document doc;
	if ( auto err = from_file( fileName, doc ) )
		return err;

	return from_document( doc, out );
}

} // namespace json5
