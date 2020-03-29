#pragma once

#include "json5.hpp"

#include <algorithm>

namespace json5 {

class object_view final
{
public:
	object_view( const value &v )
		: _pair( v.is_object() ? ( v.payload<const value *>() + 1 ) : nullptr )
		, _count( _pair ? ( _pair[-1].get<size_t>() / 2 ) : 0 )
	{ }

	using key_value_pair = std::pair<const char *, value>;

	class iterator final
	{
	public:
		iterator( const value *p = nullptr ) : _pair( p ) { }
		bool operator==( const iterator &other ) const noexcept { return _pair == other._pair; }
		iterator &operator++() { _pair += 2; return *this; }
		key_value_pair operator*() const { return key_value_pair( _pair[0].get_c_str(), _pair[1] ); }

	private:
		const value *_pair = nullptr;
	};

	iterator begin() const noexcept { return iterator( _pair ); }
	iterator end() const noexcept { return iterator( _pair + _count * 2 ); }
	iterator find( std::string_view key ) const noexcept;
	size_t size() const noexcept { return _count; }
	bool empty() const noexcept { return size() == 0; }
	value operator[]( std::string_view key ) const noexcept;

	bool operator==( const object_view &other ) const noexcept;
	bool operator!=( const object_view &other ) const noexcept { return !( ( *this ) == other ); }

private:
	const value *_pair = nullptr;
	size_t _count = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class array_view final
{
public:
	array_view( const value &v )
		: _value( v.is_array() ? ( v.payload<const value*>() + 1 ) : nullptr )
		, _count( _value ? _value[-1].get<size_t>() : 0 )
	{ }

	using iterator = const value*;

	iterator begin() const noexcept { return _value; }
	iterator end() const noexcept { return _value + _count; }
	size_t size() const noexcept { return _count; }
	bool empty() const noexcept { return _count == 0; }
	value operator[]( size_t index ) const noexcept { return _value[index]; }

	bool operator==( const array_view &other ) const noexcept;
	bool operator!=( const array_view &other ) const noexcept { return !( ( *this ) == other ); }

private:
	const value *_value = nullptr;
	size_t _count = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline object_view::iterator object_view::find( std::string_view key ) const noexcept
{
	for ( auto iter = begin(); iter != end(); ++iter )
		if ( key == ( *iter ).first )
			return iter;

	return end();
}

//---------------------------------------------------------------------------------------------------------------------
inline value object_view::operator[]( std::string_view key ) const noexcept
{
	auto iter = find( key );
	return ( iter != end() ) ? ( *iter ).second : value();
}

//---------------------------------------------------------------------------------------------------------------------
inline bool object_view::operator==( const object_view &other ) const noexcept
{
	if ( size() != other.size() )
		return false;

	if ( empty() )
		return true;

	static constexpr size_t stack_pair_count = 256;
	key_value_pair tempPairs1[stack_pair_count];
	key_value_pair tempPairs2[stack_pair_count];
	key_value_pair *pairs1 = _count <= stack_pair_count ? tempPairs1 : new key_value_pair[_count];
	key_value_pair *pairs2 = _count <= stack_pair_count ? tempPairs2 : new key_value_pair[_count];
	{ size_t i = 0; for ( auto kvp : *this ) pairs1[i++] = kvp; }
	{ size_t i = 0; for ( auto kvp : other ) pairs2[i++] = kvp; }

	auto comp = []( const key_value_pair & a, const key_value_pair & b )->bool { return strcmp( a.first, b.first ) < 0; };
	std::sort( pairs1, pairs1 + _count, comp );
	std::sort( pairs2, pairs2 + _count, comp );

	bool result = true;
	for ( size_t i = 0; i < _count; ++i )
	{
		if ( strcmp( pairs1[i].first, pairs2[i].first ) || pairs1[i].second != pairs2[i].second )
		{
			result = false;
			break;
		}
	}

	if ( _count > stack_pair_count ) { delete[] pairs1; delete[] pairs2; }
	return result;
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
