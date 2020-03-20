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

namespace json5::detail {

using string_offset = unsigned;

template <typename T> struct enum_table : std::false_type { };

} // namespace json5::detail

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace json5 {

class value;

} // namespace json5
