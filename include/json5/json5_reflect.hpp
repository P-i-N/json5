#pragma once

#include "json5.hpp"

#include <array>
#include <map>

#if !defined(JSON5_REFLECT)
#define JSON5_REFLECT(...) \
	inline auto make_named_tuple() noexcept { return std::tie( #__VA_ARGS__, __VA_ARGS__ ); } \
	inline auto make_named_tuple() const noexcept { return std::tie( #__VA_ARGS__, __VA_ARGS__ ); } \
	inline auto make_tuple() noexcept { return std::tie(__VA_ARGS__); } \
	inline auto make_tuple() const noexcept { return std::tie(__VA_ARGS__); }
#endif

namespace json5 {

namespace detail {

//---------------------------------------------------------------------------------------------------------------------
inline std::string_view get_name_slice(const char* names, size_t index)
{
	size_t numCommas = index;
	while (numCommas > 0 && *names)
		if (*names++ == ',')
			--numCommas;

	while (*names && *names <= 32)
		++names;

	size_t length = 0;
	while (names[length] > 32 && names[length] != ',')
		++length;

	return std::string_view(names, length);
}

//---------------------------------------------------------------------------------------------------------------------
inline json5::value write(builder& b, bool in) { return json5::value(in); }
inline json5::value write(builder& b, int in) { return json5::value(static_cast<double>(in)); }
inline json5::value write(builder& b, float in) { return json5::value(static_cast<double>(in)); }
inline json5::value write(builder& b, double in) { return json5::value(in); }
inline json5::value write(builder& b, const char* in) { return b.new_string(in); }
inline json5::value write(builder& b, const std::string& in) { return b.new_string(in); }

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline json5::value write_array(builder& b, const T* in, size_t numItems)
{
	b.push_array();

	for (size_t i = 0; i < numItems; ++i)
		b += write(b, in[i]);

	return b.pop();
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T, typename A>
inline json5::value write(builder& b, const std::vector<T, A>& in) { return write_array(b, in.data(), in.size()); }

//---------------------------------------------------------------------------------------------------------------------
template <typename T, size_t N>
inline json5::value write(builder& b, const T(&in)[N]) { return write_array(b, in, N); }

//---------------------------------------------------------------------------------------------------------------------
template <typename T, size_t N>
inline json5::value write(builder& b, const std::array<T, N>& in) { return write_array(b, in.data(), N); }

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline json5::value write_map(builder& b, const T& in)
{
	b.push_object();

	for (const auto& kvp : in)
		b[kvp.first] = write(b, kvp.second);

	return b.pop();
}

//---------------------------------------------------------------------------------------------------------------------
template <typename K, typename T, typename P, typename A>
inline json5::value write(builder& b, const std::map<K, T, P, A>& in) { return write_map(b, in); }

template <typename K, typename T, typename H, typename EQ, typename A>
inline json5::value write(builder& b, const std::unordered_map<K, T, H, EQ, A>& in) { return write_map(b, in); }

//---------------------------------------------------------------------------------------------------------------------
template <size_t I = 1, typename... Types>
inline void write(builder& b, const std::tuple<Types...>& t)
{
	const auto& in = std::get<I>(t);
	using Type = std::remove_reference_t<decltype(in)>;

	if (auto name = detail::get_name_slice(std::get<0>(t), I - 1); !name.empty())
	{
		if constexpr (std::is_enum_v<Type>)
			b[name] = write(b, std::underlying_type_t<Type>(in));
		else
			b[name] = write(b, in);
	}

	if constexpr (I + 1 != sizeof...(Types))
		write<I + 1>(b, t);
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline json5::value write(builder& b, const T& in)
{
	b.push_object();
	write(b, in.make_named_tuple());
	return b.pop();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline error read(const json5::value& in, bool& out)
{
	if (!in.is_boolean())
		return { error::number_expected };

	out = in.get_bool();
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error read(const json5::value& in, int& out)
{
	if (!in.is_number())
		return { error::number_expected };

	out = in.get_int();
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error read(const json5::value& in, float& out)
{
	if (!in.is_number())
		return { error::number_expected };

	out = in.get_float();
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error read(const json5::value& in, double& out)
{
	if (!in.is_number())
		return { error::number_expected };

	out = in.get_double();
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error read(const json5::value& in, const char*& out)
{
	if (!in.is_string())
		return { error::string_expected };

	out = in.get_c_str();
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error read(const json5::value& in, std::string& out)
{
	if (!in.is_string())
		return { error::string_expected };

	out = in.get_c_str();
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error read_array(const json5::value& in, T* out, size_t numItems)
{
	if (!in.is_array())
		return { error::array_expected };

	auto arr = json5::array(in);
	if (arr.size() != numItems)
		return { error::wrong_array_size };

	for (size_t i = 0; i < numItems; ++i)
		if (auto err = read(arr[i], out[i]))
			return err;

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T, size_t N>
inline error read(const json5::value& in, T(&out)[N]) { return read_array(in, out, N); }

//---------------------------------------------------------------------------------------------------------------------
template <typename T, size_t N>
inline error read(const json5::value& in, std::array<T, N>& out) { return read_array(in, out.data(), N); }

//---------------------------------------------------------------------------------------------------------------------
template <typename T, typename A>
inline error read(const json5::value& in, std::vector<T, A>& out)
{
	if (!in.is_array() && !in.is_null())
		return { error::array_expected };

	auto arr = json5::array(in);

	out.clear();
	out.reserve(arr.size());
	for (const auto& i : arr)
		if (auto err = read(i, out.emplace_back()))
			return err;

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error read_map(const json5::value& in, T& out)
{
	if (!in.is_object() && !in.is_null())
		return { error::object_expected };

	std::pair<typename T::key_type, typename T::mapped_type> kvp;

	out.clear();
	for (auto jsKV : json5::object(in))
	{
		kvp.first = jsKV.first;

		if (auto err = read(jsKV.second, kvp.second))
			return err;

		out.emplace(std::move(kvp));
	}

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
template <typename K, typename T, typename P, typename A>
inline error read(const json5::value& in, std::map<K, T, P, A>& out) { return read_map(in, out); }

template <typename K, typename T, typename H, typename EQ, typename A>
inline error read(const json5::value& in, std::unordered_map<K, T, H, EQ, A>& out) { return read_map(in, out); }

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error read(const json5::value& in, T& out)
{
	if (!in.is_object())
		return { error::object_expected };

	return read(json5::object(in), out.make_named_tuple());
}

//---------------------------------------------------------------------------------------------------------------------
template <size_t I = 1, typename... Types>
inline error read(const json5::object& obj, std::tuple<Types...>& t)
{
	auto& out = std::get<I>(t);
	using Type = std::remove_reference_t<decltype(out)>;

	auto name = get_name_slice(std::get<0>(t), I - 1);

	auto iter = obj.find(name);
	if (iter != obj.end())
	{
		if (auto err = read((*iter).second, out))
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
inline void to_document(document& doc, const T& value)
{
	builder b(doc);
	detail::write(b, value);
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void to_stream(std::ostream& os, const T& value)
{
	document doc;
	to_document(doc, value);
	to_stream(os, doc);
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void to_string(std::string& str, const T& value)
{
	document doc;
	to_document(doc, value);
	to_string(str, doc);
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
