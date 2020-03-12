#pragma once

#include "json5.hpp"

#if !defined(JSON5_REFLECT)
#define JSON5_REFLECT(...) \
	auto make_tuple() noexcept { return std::tie( #__VA_ARGS__, __VA_ARGS__ ); } \
	auto make_tuple() const noexcept { return std::tie( #__VA_ARGS__, __VA_ARGS__ ); }
#endif

namespace json5 {

namespace detail {

struct write_context final
{
	std::ostream& os;
	int depth = 0;

	void indent() { for (int i = 0; i < depth; ++i) os << "    "; }
};

//---------------------------------------------------------------------------------------------------------------------
inline std::string_view get_name_slice(const char* names, size_t index)
{
	size_t numCommas = index;
	while (numCommas > 0 && *names)
		if (*names++ == ',')
			--numCommas;

	while (*names <= 32)
		++names;

	size_t length = 0;
	while (names[length] > 32 && names[length] != ',')
		++length;

	return std::string_view(names, length);
}

//---------------------------------------------------------------------------------------------------------------------
inline void to_string(write_context& ctx, int value)
{
	ctx.os << value;
}

//---------------------------------------------------------------------------------------------------------------------
inline void to_string(write_context& ctx, const char* value)
{
	ctx.os << "\"";

	while (*value)
	{
		// TODO: Properly escape!
		char ch = *value;
		ctx.os << ch;

		++value;
	}

	ctx.os << "\"";
}

//---------------------------------------------------------------------------------------------------------------------
inline void to_string(write_context& ctx, const std::string& value) { to_string(ctx, value.c_str()); }

//---------------------------------------------------------------------------------------------------------------------
template <size_t I = 1, typename... Types>
inline void to_string(write_context& ctx, const std::tuple<Types...>& t)
{
	const auto& value = std::get<I>(t);
	using Type = std::remove_reference_t<decltype(value)>;

	auto name = get_name_slice(std::get<0>(t), I - 1);

	ctx.indent();
	ctx.os << name << ": ";

	if constexpr (std::is_enum_v<Type>)
		to_string(ctx, std::underlying_type_t<Type>(value));
	else
		to_string(ctx, value);

	if constexpr (I + 1 != sizeof...(Types))
		ctx.os << ",";

	ctx.os << std::endl;

	if constexpr (I + 1 != sizeof...(Types))
		to_string<I + 1>(ctx, t);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline error from_string(const value& jsValue, int& result)
{
	if (!jsValue.is_number())
		return { error::number_expected };

	result = jsValue.get_int();
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error from_string(const value& jsValue, const char*& result)
{
	if (!jsValue.is_string())
		return { error::string_expected };

	result = jsValue.get_c_str();
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
template <size_t I = 1, typename... Types>
inline error from_string(const object& obj, std::tuple<Types...>& t)
{
	auto& value = std::get<I>(t);
	using Type = std::remove_reference_t<decltype(value)>;

	auto name = get_name_slice(std::get<0>(t), I - 1);

	auto iter = obj.find(name);
	if (iter != obj.end())
	{
		if (auto err = from_string((*iter).second, value))
			return err;
	}

	if constexpr (I + 1 != sizeof...(Types))
	{
		if (auto err = from_string<I + 1>(obj, t))
			return err;
	}

	return { error::none };
}

} // namespace detail

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void to_string(detail::write_context& ctx, const T& value)
{
	ctx.os << "{\n";
	++ctx.depth;

	detail::to_string(ctx, value.make_tuple());

	--ctx.depth;
	ctx.indent();
	ctx.os << "}\n";
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void to_string(std::ostream& os, const T& value)
{
	detail::write_context ctx{ os };
	to_string(ctx, value);
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void to_string(std::string& str, const T& value)
{
	std::ostringstream os;
	to_string(os, value);
	str = os.str();
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline std::string to_string(const T& value)
{
	std::string result;
	to_string(result, value);
	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error from_string(const document& doc, T& value)
{
	if (!doc.root().is_object())
		return { error::object_expected };

	return detail::from_string(object(doc.root()), value.make_tuple());
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error from_string(const std::string& str, T& value)
{
	document doc;
	if (auto err = doc.parse(str))
		return err;

	return from_string(doc, value);
}

} // json5
