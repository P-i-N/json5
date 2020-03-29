#pragma once

#include <tuple>

#if !defined(JSON5_REFLECT)
#define JSON5_REFLECT(...) \
	inline auto make_named_tuple() noexcept { return std::tie( #__VA_ARGS__, __VA_ARGS__ ); } \
	inline auto make_named_tuple() const noexcept { return std::tie( #__VA_ARGS__, __VA_ARGS__ ); } \
	inline auto make_tuple() noexcept { return std::tie(__VA_ARGS__); } \
	inline auto make_tuple() const noexcept { return std::tie(__VA_ARGS__); }
#endif

#if !defined(JSON5_ENUM)
#define JSON5_ENUM(_Name, ...) \
	template <> struct json5::detail::enum_table<_Name> : std::true_type { \
		using enum _Name; \
		static constexpr char* names = #__VA_ARGS__; \
		static constexpr _Name values[] = { __VA_ARGS__ }; };
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace json5 {

/* Forward declarations */
class document;
class value;

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
	};

	static constexpr char *type_string[] =
	{
		"none", "invalid root", "unexpected end", "syntax error", "invalid literal",
		"invalid escape sequence", "comma expected", "colon expected", "boolean expected",
		"number expected", "string expected", "object expected", "array expected",
		"wrong array size", "invalid enum"
	};

	int type = none;
	int line = 0;
	int column = 0;

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
template <typename T> struct enum_table : std::false_type { };

} // namespace json5::detail
