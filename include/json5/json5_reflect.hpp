#pragma once

#include "json5.hpp"

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
	const char* scope_chars;

	writer_scope(writer& w, const char* ch): wr(w), scope_chars(ch)
	{
		wr.os << scope_chars[0] << std::endl;
		++wr.depth;
	}

	~writer_scope()
	{
		--wr.depth;
		wr.indent();
		wr.os << scope_chars[1];
	}
};

template <typename T> struct inherits final { using type = T; };

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
inline void write(writer& w, bool value) { w.os << (value ? "true" : "false"); }

//---------------------------------------------------------------------------------------------------------------------
inline void write(writer& w, int value) { w.os << value; }
inline void write(writer& w, float value) { w.os << value; }
inline void write(writer& w, double value) { w.os << value; }

//---------------------------------------------------------------------------------------------------------------------
inline void write(writer& w, const char* value) { json5::to_stream(w.os, value); }
inline void write(writer& w, const std::string& value) { write(w, value.c_str()); }

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void write_array(writer& w, const T* values, size_t numItems)
{
	if (numItems)
	{
		writer_scope _(w, "[]");

		for (size_t i = 0; i < numItems; ++i)
		{
			w.indent();
			write(w, values[i]);

			if (i < numItems - 1) w.os << ",";
			w.os << std::endl;
		}
	}
	else
		w.os << "[]";
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T, typename A>
inline void write(writer& w, const std::vector<T, A>& value) { write_array(w, value.data(), value.size()); }

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void write_map(writer& w, const T& value)
{
	if (!value.empty())
	{
		writer_scope _(w, "{}");

		size_t count = value.size();
		for (const auto& kvp : value)
		{
			w.indent();
			write(w, kvp.first);
			w.os << ": ";
			write(w, kvp.second);

			if (--count) w.os << ",";
			w.os << std::endl;
		}
	}
	else
		w.os << "{}";
}

//---------------------------------------------------------------------------------------------------------------------
template <typename K, typename T, typename P, typename A>
inline void write(writer& w, const std::map<K, T, P, A>& value) { write_map(w, value); }

template <typename K, typename T, typename H, typename EQ, typename A>
inline void write(writer& w, const std::unordered_map<K, T, H, EQ, A>& value) { write_map(w, value); }

//---------------------------------------------------------------------------------------------------------------------
template <size_t I = 1, typename... Types>
inline void write(writer& w, const std::tuple<Types...>& t)
{
	const auto& value = std::get<I>(t);
	using Type = std::remove_reference_t<decltype(value)>;

	auto name = get_name_slice(std::get<0>(t), I - 1);

	if (!name.empty())
	{
		w.indent();
		w.os << name << ": ";

		if constexpr (std::is_enum_v<Type>)
			write(w, std::underlying_type_t<Type>(value));
		else
			write(w, value);

		if constexpr (I + 1 != sizeof...(Types))
			w.os << ",";

		w.os << std::endl;
	}

	if constexpr (I + 1 != sizeof...(Types))
		write<I + 1>(w, t);
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void write(writer& w, const T& value)
{
	{
		writer_scope _(w, "{}");
		write(w, value.make_named_tuple());
	}

	if (!w.depth)
		w.os << std::endl;
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
inline void to_stream(std::ostream& os, const T& value)
{
	detail::write(detail::writer{ os }, value);
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
