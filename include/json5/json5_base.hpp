#pragma once

#if !defined(JSON5_ASSERT)
	#include <cassert>
	#define JSON5_ASSERT(_Cond) assert(_Cond)
#endif

/*
	Generates class serialization helper for specified type:

	namespace foo {
		struct Bar { int x; float y; bool z; };
	}

	JSON5_CLASS(foo::Bar, x, y, z)
*/
#define JSON5_CLASS(_Name, ...) \
	template <> struct json5::detail::class_wrapper<_Name> { \
		static constexpr const char* names = #__VA_ARGS__; \
		inline static auto make_named_ref_list( _Name &out ) noexcept { \
			auto &[__VA_ARGS__] = out; \
			return json5::detail::named_ref_list( names, __VA_ARGS__ ); \
		} \
		inline static auto make_named_ref_list( const _Name &in ) noexcept { \
			const auto &[__VA_ARGS__] = in; \
			return json5::detail::named_ref_list( names, __VA_ARGS__ ); \
		} \
	};

/*
	Generates members serialization helper inside class:

	namespace foo {
		struct Bar {
			int x; float y; bool z;
			JSON5_MEMBERS(x, y, z)
		};
	}
*/
#define JSON5_MEMBERS(...) \
	inline auto make_named_ref_list() noexcept { \
		return json5::detail::named_ref_list((const char*)#__VA_ARGS__, __VA_ARGS__); } \
	inline auto make_named_ref_list() const noexcept { \
		return json5::detail::named_ref_list((const char*)#__VA_ARGS__, __VA_ARGS__); }

/*
	Generates enum wrapper:

	enum class MyEnum {
		One, Two, Three
	};

	JSON5_ENUM(MyEnum, One, Two, Three)
*/
#define JSON5_ENUM(_Name, ...) \
	template <> struct json5::detail::enum_table<_Name> : json5::detail::true_type { \
		using enum _Name; \
		static constexpr const char* names = #__VA_ARGS__; \
		static constexpr const _Name values[] = { __VA_ARGS__ }; };

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace json5 {

/* Forward declarations */
class builder;
class document;
class parser;
class value;

//---------------------------------------------------------------------------------------------------------------------
struct location final
{
	unsigned line   : 24; // Source line number (1 = first, 0 = unknown line)
	unsigned column :  8; // Source column number (1 = first, 0 = unknown column)
	unsigned offset : 32; // Byte offset

	location(): line( 0 ), column( 0 ), offset( 0 ) { }
	location( unsigned l, unsigned c, unsigned off ): line( l ), column( c ), offset( off ) { }

	bool is_valid() const noexcept { return line > 0 && column > 0; }
};

//---------------------------------------------------------------------------------------------------------------------
struct error final
{
	enum
	{
		none,               // no error
		invalid_root,       // document root is not an object or array
		unexpected_end,     // unexpected end of JSON data (end of stream, string or file)
		syntax_error,       // general parsing error
		invalid_literal,    // invalid literal, only "true", "false", "null" allowed
		invalid_escape_seq, // invalid or unsupported string escape \ sequence
		comma_expected,     // expected comma ','
		colon_expected,     // expected color ':'
		boolean_expected,   // expected boolean literal "true" or "false"
		number_expected,    // expected number
		string_expected,    // expected string "..."
		object_expected,    // expected object { ... }
		array_expected,     // expected array [ ... ]
		wrong_array_size,   // invalid number of array elements
		invalid_enum,       // invalid enum value or string (conversion failed)
		could_not_open,     // stream is not open
	};

	static constexpr const char *type_string[] =
	{
		"none", "invalid root", "unexpected end", "syntax error", "invalid literal",
		"invalid escape sequence", "comma expected", "colon expected", "boolean expected",
		"number expected", "string expected", "object expected", "array expected",
		"wrong array size", "invalid enum", "could not open stream",
	};

	int type = none;
	location loc = { };

	error() = default;
	error( int errType ) : type( errType ) { }
	error( int errType, location l ): type( errType ), loc( l ) { }

	operator int() const noexcept { return type; }
};

//---------------------------------------------------------------------------------------------------------------------
struct writer_params
{
	// One level of indentation
	const char *indentation = "  ";

	// End of line string
	const char *eol = "\n";

	// Write all on single line, omit extra spaces
	bool compact = false;

	// Arrays of this size or less will be put on single line
	size_t compact_array_size = 5;

	// Objects of this size of less will be put on single line
	size_t compact_object_size = 1;

	// Write regular JSON (don't use any JSON5 features)
	bool json_compatible = false;

	// Escape unicode characters in strings
	bool escape_unicode = false;

	// Custom user data pointer
	void *user_data = nullptr;
};

//---------------------------------------------------------------------------------------------------------------------
enum class value_type { null = 0, boolean, number, array, string, object };

} // namespace json5

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace json5::detail {

using string_offset = unsigned;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <size_t I> struct index { static constexpr size_t value = I; };

template <typename... Args> class ref_list { };

template <> class ref_list<>
{
public:
	ref_list() { }
};

template <typename Head, typename... Tail>
class ref_list<Head, Tail...> : public ref_list<Tail... >
{
	Head &_value;

public:
	using base_t = ref_list<Tail...>;

	static constexpr size_t length = sizeof...( Tail ) + 1;

	template <size_t I> auto &get( index<I> tag ) { return base_t::get( index < I - 1 > () ); }
	Head &get( index<0> ) { return _value; }

	template <size_t I> const auto &get( index<I> tag ) const { return base_t::get( index < I - 1 > () ); }
	const Head &get( index<0> ) const { return _value; }

	ref_list( Head &head, Tail &... tail ): base_t( tail... ), _value( head ) { }
};

template <typename... Args> class named_ref_list : public ref_list<Args...>
{
	const char *_names = nullptr;

public:
	using base_t = ref_list<Args...>;

	named_ref_list( const char *argNames, Args &... args ): base_t( args... ), _names( argNames ) { }

	const char *names() const noexcept { return _names; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> struct class_wrapper
{
	inline static auto make_named_ref_list( T &in ) noexcept { return in.make_named_ref_list(); }
	inline static auto make_named_ref_list( const T &in ) noexcept { return in.make_named_ref_list(); }
};

struct true_type { constexpr operator bool() const noexcept { return true; } };
struct false_type { constexpr operator bool() const noexcept { return false; } };

template <typename T> struct enum_table : false_type { };

} // namespace json5::detail
