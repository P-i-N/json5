#pragma once

#include "json5.hpp"

#include <map>

#if !defined(JSON5_REFLECT)
#define JSON5_REFLECT(...) \
	auto make_named_tuple() noexcept { return std::tie( #__VA_ARGS__, __VA_ARGS__ ); } \
	auto make_named_tuple() const noexcept { return std::tie( #__VA_ARGS__, __VA_ARGS__ ); } \
	auto make_tuple() noexcept { return std::tie(__VA_ARGS__); } \
	auto make_tuple() const noexcept { return std::tie(__VA_ARGS__); }
#endif

namespace json5 {

namespace detail {

struct writer final
{
	std::ostream& os;
	int depth = 0;
	const char* indent_str = "  ";

	void indent() { for (int i = 0; i < depth; ++i) os << indent_str; }
};

struct writer_scope final
{
	writer& wr;
	const char* chars;

	writer_scope(writer& w, const char* ch): wr(w), chars(ch)
	{
		wr.os << chars[0] << std::endl;
		++wr.depth;
	}

	~writer_scope()
	{
		--wr.depth;
		wr.indent();
		wr.os << chars[1];
	}
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
inline void write(writer& ctx, bool value) { ctx.os << (value ? "true" : "false"); }

//---------------------------------------------------------------------------------------------------------------------
inline void write(writer& ctx, int value) { ctx.os << value; }
inline void write(writer& ctx, float value) { ctx.os << value; }
inline void write(writer& ctx, double value) { ctx.os << value; }

//---------------------------------------------------------------------------------------------------------------------
inline void write(writer& ctx, const char* value) { json5::to_stream(ctx.os, value); }
inline void write(writer& ctx, const std::string& value) { write(ctx, value.c_str()); }

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void write_array(writer& ctx, const T* values, size_t numItems)
{
	if (numItems)
	{
		writer_scope _(ctx, "[]");

		for (size_t i = 0; i < numItems; ++i)
		{
			ctx.indent();
			write(ctx, values[i]);

			if (i < numItems - 1) ctx.os << ",";
			ctx.os << std::endl;
		}
	}
	else
		ctx.os << "[]";
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T, typename A>
inline void write(writer& ctx, const std::vector<T, A>& value) { write_array(ctx, value.data(), value.size()); }

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void write_map(writer& ctx, const T& value)
{
	if (!value.empty())
	{
		writer_scope _(ctx, "{}");

		size_t count = value.size();
		for (const auto& kvp : value)
		{
			ctx.indent();
			write(ctx, kvp.first);
			ctx.os << ": ";
			write(ctx, kvp.second);

			if (--count) ctx.os << ",";
			ctx.os << std::endl;
		}
	}
	else
		ctx.os << "{}";
}

//---------------------------------------------------------------------------------------------------------------------
template <typename K, typename T, typename P, typename A>
inline void write(writer& ctx, const std::map<K, T, P, A>& value) { write_map(ctx, value); }

template <typename K, typename T, typename H, typename EQ, typename A>
inline void write(writer& ctx, const std::unordered_map<K, T, H, EQ, A>& value) { write_map(ctx, value); }

//---------------------------------------------------------------------------------------------------------------------
template <size_t I = 1, typename... Types>
inline void write(writer& ctx, const std::tuple<Types...>& t)
{
	const auto& value = std::get<I>(t);
	using Type = std::remove_reference_t<decltype(value)>;

	auto name = get_name_slice(std::get<0>(t), I - 1);

	ctx.indent();
	ctx.os << name << ": ";

	if constexpr (std::is_enum_v<Type>)
		write(ctx, std::underlying_type_t<Type>(value));
	else
		write(ctx, value);

	if constexpr (I + 1 != sizeof...(Types))
		ctx.os << ",";

	ctx.os << std::endl;

	if constexpr (I + 1 != sizeof...(Types))
		write<I + 1>(ctx, t);
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void write(writer& ctx, const T& value)
{
	{
		writer_scope _(ctx, "{}");
		write(ctx, value.make_named_tuple());
	}

	if (!ctx.depth)
		ctx.os << std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline error read(const value& jsValue, bool& result)
{
	if (!jsValue.is_boolean())
		return { error::number_expected };

	result = jsValue.get_bool();
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error read(const value& jsValue, int& result)
{
	if (!jsValue.is_number())
		return { error::number_expected };

	result = jsValue.get_int();
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error read(const value& jsValue, float& result)
{
	if (!jsValue.is_number())
		return { error::number_expected };

	result = jsValue.get_float();
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error read(const value& jsValue, double& result)
{
	if (!jsValue.is_number())
		return { error::number_expected };

	result = jsValue.get_double();
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error read(const value& jsValue, const char*& result)
{
	if (!jsValue.is_string())
		return { error::string_expected };

	result = jsValue.get_c_str();
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error read(const value& jsValue, std::string& result)
{
	if (!jsValue.is_string())
		return { error::string_expected };

	result = jsValue.get_c_str();
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T, typename A>
inline error read(const value& jsValue, std::vector<T, A>& result)
{
	if (!jsValue.is_array() && !jsValue.is_null())
		return { error::array_expected };

	auto arr = json5::array(jsValue);

	result.clear();
	result.reserve(arr.size());
	for (const auto& i : arr)
		if (auto err = read(i, result.emplace_back()))
			return err;

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error read_map(const value& jsValue, T& result)
{
	if (!jsValue.is_object() && !jsValue.is_null())
		return { error::object_expected };

	std::pair<typename T::key_type, typename T::mapped_type> kvp;

	result.clear();
	for (auto jsKV : json5::object(jsValue))
	{
		kvp.first = jsKV.first;

		if (auto err = read(jsKV.second, kvp.second))
			return err;

		result.emplace(std::move(kvp));
	}

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
template <typename K, typename T, typename P, typename A>
inline error read(const value& jsValue, std::map<K, T, P, A>& result) { return read_map(jsValue, result); }

template <typename K, typename T, typename H, typename EQ, typename A>
inline error read(const value& jsValue, std::unordered_map<K, T, H, EQ, A>& result) { return read_map(jsValue, result); }

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error read(const value& jsValue, T& result)
{
	if (!jsValue.is_object())
		return { error::object_expected };

	return read(json5::object(jsValue), result.make_named_tuple());
}

//---------------------------------------------------------------------------------------------------------------------
template <size_t I = 1, typename... Types>
inline error read(const object& obj, std::tuple<Types...>& t)
{
	auto& value = std::get<I>(t);
	using Type = std::remove_reference_t<decltype(value)>;

	auto name = get_name_slice(std::get<0>(t), I - 1);

	auto iter = obj.find(name);
	if (iter != obj.end())
	{
		if (auto err = read((*iter).second, value))
			return err;
	}

	if constexpr (I + 1 != sizeof...(Types))
	{
		if (auto err = read<I + 1>(obj, t))
			return err;
	}

	return { error::none };
}

} // namespace detail

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void to_stream(detail::writer& ctx, const T& value)
{
	detail::write(ctx, value);
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void to_stream(std::ostream& os, const T& value)
{
	detail::writer ctx{ os };
	detail::write(ctx, value);
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void to_string(std::string& str, const T& value)
{
	std::ostringstream os;
	to_stream(os, value);
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

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline bool to_file(const std::string& fileName, const T& value)
{
	std::ofstream ofs(fileName);
	to_stream(ofs, value);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error from_document(const document& doc, T& value)
{
	return detail::read(doc.root(), value);
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error from_string(const std::string& str, T& value)
{
	document doc;
	if (auto err = from_string(str, doc))
		return err;

	return from_document(doc, value);
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error from_file(const std::string& fileName, T& value)
{
	document doc;
	if (auto err = from_file(fileName, doc))
		return err;

	return from_document(doc, value);
}

} // json5
