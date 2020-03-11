#pragma once

#include <charconv>
#include <cstdint>
#include <istream>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#if !defined(JSON5_REFLECT)
#define JSON5_REFLECT(...) \
	auto make_tuple() noexcept { return std::tie( #__VA_ARGS__, __VA_ARGS__ ); } \
	auto make_tuple() const noexcept { return std::tie( #__VA_ARGS__, __VA_ARGS__ ); }
#endif

namespace json5::detail {

struct hashed_string_ref
{
	size_t hash = 0;
	size_t offset = 0;

	bool operator==(const hashed_string_ref& other) const noexcept { return hash == other.hash; }
};

} // namespace json5::detail

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace std {

template <> struct hash<json5::detail::hashed_string_ref>
{
	size_t operator()(const json5::detail::hashed_string_ref& value) const noexcept { return value.hash; }
};

} // namespace std

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace json5 {

/* Forward declarations*/
class document;

enum class value_type { null, boolean, string, number, object, array };

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class value
{
public:
	virtual ~value() = default;

	value_type type() const noexcept { return _type; }

protected:
	value(document& doc, value_type type) : _doc(doc), _type(type) { }

	const char* c_str_offset(size_t offset) const noexcept;

	document& _doc;
	value_type _type = value_type::null;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class number final : public value
{
public:
	number(document& doc): value(doc, value_type::number) { }

	int64_t as_integer() const noexcept { return static_cast<int64_t>(_double); }

	double as_double() const noexcept { return _double; }

	float as_float() const noexcept { return static_cast<float>(_double); }

	const char* c_str() const noexcept { return c_str_offset(_stringOffset); }

private:
	double _double = 0.0;
	size_t _stringOffset = 0;

	friend class document;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class string final : public value
{
public:
	string(document& doc): value(doc, value_type::string) { }

	const char* c_str() const noexcept { return c_str_offset(_stringOffset); }

private:
	size_t _stringOffset = 0;

	friend class document;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class object final : public value
{
	using property_map = std::unordered_map<detail::hashed_string_ref, std::unique_ptr<value>>;

public:
	object(document& doc): value(doc, value_type::object) { }

	class iterator final
	{
	public:
		iterator(property_map::const_iterator iter, const char *strBuff): _iter(iter), _stringBuffer(strBuff) { }

		bool operator==(const iterator& other) const noexcept { return _iter == other._iter; }

		iterator& operator++() { ++_iter; return *this; }
		
		std::pair<const char *, const value&> operator*() const
		{
			return { _stringBuffer + _iter->first.offset, *_iter->second };
		}

	private:
		property_map::const_iterator _iter;
		const char* _stringBuffer;
	};

	iterator begin() const noexcept;

	iterator end() const noexcept;

	iterator find(std::string_view key) const noexcept;

	bool empty() const noexcept { return _properties.empty(); }

	bool contains(std::string_view key) const noexcept;

private:
	property_map _properties;

	friend class document;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class array final : public value
{
public:
	array(document& doc) : value(doc, value_type::array) { }

	size_t size() const noexcept { return _values.size(); }

	const value& operator[](size_t index) const { return *(_values[index]); }

private:
	std::vector<std::unique_ptr<value>> _values;

	friend class document;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct error final
{
	enum
	{
		none,
		invalid_root,
		unexpected_end,
		syntax_error,
		comma_expected,
	};

	int value = none;
	int line = 0;
	int column = 0;

	operator int() const noexcept { return value; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class document final
{
public:
	document();

	error parse(std::istream& is) { return parse(context{ is }); }

	error parse(const std::string& str) { return parse(context{ std::istringstream(str) }); }

	const object& root() const noexcept { return _root; }

private:
	struct context final
	{
		std::istream& is;
		int line = 1;
		int column = 1;

		char next()
		{
			char ch = is.get();

			++column;
			if (ch == '\n')
			{
				column = 1;
				++line;
			}

			return ch;
		}

		char peek() { return is.peek(); }

		bool eof() const { return is.eof(); }

		error to_error(int type) const noexcept { return error{ type, line, column }; }
	};

	error parse(context& ctx);

	error parse(context& ctx, std::unique_ptr<value>& result);

	error parse(context& ctx, object& result);

	error parse(context& ctx, number& result);

	error parse(context& ctx, string& result);

	error parse(context& ctx, array& result);

	enum class token_type
	{
		unknown, identifier, string, number, colon, comma, object_begin, object_end, array_begin, array_end,
	};

	error peek_next_token(context& ctx, token_type& result);

	error parse_identifier(context& ctx, size_t& result);

	std::vector<char> _stringBuffer;

	object _root;

	friend class value;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
	if (jsValue.type() == value_type::number)
		result = static_cast<int>(static_cast<const number&>(jsValue).as_integer());

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error from_string(const value& jsValue, const char*& result)
{
	if (jsValue.type() == value_type::string)
		result = static_cast<const string&>(jsValue).c_str();

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

template <typename T>
inline void to_string(std::ostream& os, const T& value)
{
	detail::write_context ctx{ os };
	to_string(ctx, value);
}

template <typename T>
inline void to_string(std::string& str, const T& value)
{
	std::ostringstream os;
	to_string(os, value);
	str = os.str();
}

template <typename T>
inline std::string to_string(const T& value)
{
	std::string result;
	to_string(result, value);
	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline error from_string(const document& doc, T& value)
{
	return detail::from_string(doc.root(), value.make_tuple());
}

template <typename T>
inline error from_string(const std::string& str, T& value)
{
	document doc;
	if (auto err = doc.parse(str))
		return err;

	return from_string(doc, value);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline const char* value::c_str_offset(size_t offset) const noexcept
{
	return _doc._stringBuffer.data() + offset;
}

//---------------------------------------------------------------------------------------------------------------------
inline object::iterator object::begin() const noexcept
{
	return iterator(_properties.begin(), c_str_offset(0));
}

//---------------------------------------------------------------------------------------------------------------------
inline object::iterator object::end() const noexcept
{
	return iterator(_properties.end(), c_str_offset(0));
}

//---------------------------------------------------------------------------------------------------------------------
inline object::iterator object::find(std::string_view key) const noexcept
{
	auto hash = std::hash<std::string_view>()(key);
	return iterator(_properties.find({ hash }), c_str_offset(0));
}

//---------------------------------------------------------------------------------------------------------------------
inline bool object::contains(std::string_view key) const noexcept
{
	auto hash = std::hash<std::string_view>()(key);
	return _properties.contains({ hash });
}

//---------------------------------------------------------------------------------------------------------------------
inline document::document()
	: _root(*this)
{

}

//---------------------------------------------------------------------------------------------------------------------
inline error document::parse(context& ctx)
{
	_stringBuffer.reserve(8192); // Reserve 8kB for strings
	_stringBuffer.clear();
	_stringBuffer.push_back(0);

	token_type tt = token_type::unknown;
	if (auto err = peek_next_token(ctx, tt))
		return err;
	
	if (tt != token_type::object_begin)
		return ctx.to_error(error::invalid_root);
	
	return parse(ctx, _root);
}

//---------------------------------------------------------------------------------------------------------------------
inline error document::parse(context& ctx, std::unique_ptr<value>& result)
{
	token_type tt = token_type::unknown;
	if (auto err = peek_next_token(ctx, tt))
		return err;

	switch (tt)
	{
		case token_type::number:
		{
			std::unique_ptr<number> numberResult = std::make_unique<number>(*this);
			if (auto err = parse(ctx, *numberResult))
				return err;

			result = std::move(numberResult);
			return { error::none };
		}

		case token_type::string:
		{
			std::unique_ptr<string> stringResult = std::make_unique<string>(*this);
			if (auto err = parse(ctx, *stringResult))
				return err;

			result = std::move(stringResult);
			return { error::none };
		}

		case token_type::object_begin:
		{
			std::unique_ptr<object> objectResult = std::make_unique<object>(*this);
			if (auto err = parse(ctx, *objectResult))
				return err;

			result = std::move(objectResult);
			return { error::none };
		}

		case token_type::array_begin:
		{
			std::unique_ptr<array> arrayResult = std::make_unique<array>(*this);
			if (auto err = parse(ctx, *arrayResult))
				return err;

			result = std::move(arrayResult);
			return { error::none };
		}
	}

	return ctx.to_error(error::syntax_error);
}

//---------------------------------------------------------------------------------------------------------------------
inline error document::parse(context& ctx, object& result)
{
	ctx.next(); // Consume '{'

	bool expectComma = false;
	while (!ctx.eof())
	{
		token_type tt = token_type::unknown;
		if (auto err = peek_next_token(ctx, tt))
			return err;

		size_t keyOffset;

		switch (tt)
		{
			case token_type::identifier:
			case token_type::string:
			{
				if (expectComma)
					return ctx.to_error(error::comma_expected);

				if (auto err = parse_identifier(ctx, keyOffset))
					return err;
			}
			break;

			case token_type::object_end:
				ctx.next(); // Consume '}'
				return { error::none };

			case token_type::comma:
			{
				if (!expectComma)
					return ctx.to_error(error::syntax_error);

				ctx.next(); // Consume ','
				expectComma = false;
				continue;
			}

			default:
				return expectComma ? ctx.to_error(error::comma_expected) : ctx.to_error(error::syntax_error);
		}
		
		if (auto err = peek_next_token(ctx, tt))
			return err;

		if (tt != token_type::colon)
			return ctx.to_error(error::syntax_error);

		ctx.next(); // Consume ':'

		std::unique_ptr<value> valuePtr;
		if (auto err = parse(ctx, valuePtr))
			return err;

		detail::hashed_string_ref hashedKey;
		hashedKey.offset = keyOffset;

		auto sv = std::string_view(_stringBuffer.data() + hashedKey.offset);
		hashedKey.hash = std::hash<std::string_view>()(sv);

		result._properties.insert({ hashedKey, std::move(valuePtr) });
		expectComma = true;
	}

	return ctx.to_error(error::unexpected_end);
}

//---------------------------------------------------------------------------------------------------------------------
inline error document::parse(context& ctx, number& result)
{
	result._stringOffset = _stringBuffer.size();
	size_t length = 0;

	while (!ctx.eof())
	{
		_stringBuffer.push_back(ctx.next());
		++length;

		char ch = ctx.peek();
		if (!isdigit(ch))
			break;
	}

	_stringBuffer.push_back(0);

	auto convResult = std::from_chars(
		_stringBuffer.data() + result._stringOffset,
		_stringBuffer.data() + result._stringOffset + length,
		result._double);

	if (convResult.ec != std::errc())
		return ctx.to_error(error::syntax_error);

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error document::parse(context& ctx, string& result)
{
	bool singleQuoted = ctx.peek() == '\'';
	ctx.next(); // Consume '\'' or '"'

	result._stringOffset = _stringBuffer.size();

	while (!ctx.eof())
	{
		char ch = ctx.peek();
		if ((singleQuoted && ch == '\'') || (!singleQuoted && ch == '"'))
		{
			ctx.next(); // Consume '\'' or '"'
			break;
		}

		_stringBuffer.push_back(ctx.next());
	}

	if (ctx.eof())
		return ctx.to_error(error::unexpected_end);

	_stringBuffer.push_back(0);
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error document::parse(context& ctx, array& result)
{
	ctx.next(); // Consume '['

	bool expectComma = false;
	while (!ctx.eof())
	{
		token_type tt = token_type::unknown;
		if (auto err = peek_next_token(ctx, tt))
			return err;

		if (tt == token_type::array_end)
		{
			ctx.next(); // Consume ']'
			return { error::none };
		}
		else if (expectComma)
		{
			expectComma = false;

			if (tt != token_type::comma)
				return ctx.to_error(error::comma_expected);

			ctx.next(); // Consume ','
			continue;
		}

		std::unique_ptr<value> valuePtr;
		if (auto err = parse(ctx, valuePtr))
			return err;

		result._values.push_back(std::move(valuePtr));
		expectComma = true;
	}

	return ctx.to_error(error::unexpected_end);
}

//---------------------------------------------------------------------------------------------------------------------
inline error document::peek_next_token(context& ctx, token_type& result)
{
	bool parsingComment = false;

	while (!ctx.eof())
	{
		char ch = ctx.peek();
		if (ch == '\n')
			parsingComment = false;
		else if (parsingComment || ch <= 32)
		{
			/* Do nothing */
		}
		else if (ch == '/')
		{
			ctx.next();

			if (ctx.peek() != '/')
				return ctx.to_error(error::syntax_error);

			parsingComment = true;
		}
		else if (ch == '{')
		{
			result = token_type::object_begin;
			return { error::none };
		}
		else if (ch == '}')
		{
			result = token_type::object_end;
			return { error::none };
		}
		else if (ch == '[')
		{
			result = token_type::array_begin;
			return { error::none };
		}
		else if (ch == ']')
		{
			result = token_type::array_end;
			return { error::none };
		}
		else if (ch == ':')
		{
			result = token_type::colon;
			return { error::none };
		}
		else if (ch == ',')
		{
			result = token_type::comma;
			return { error::none };
		}
		else if (isalpha(ch) || ch == '_')
		{
			result = token_type::identifier;
			return { error::none };
		}
		else if (isdigit(ch) || ch == '.' || ch == '+' || ch == '-')
		{
			result = token_type::number;
			return { error::none };
		}
		else if (ch == '"' || ch == '\'')
		{
			result = token_type::string;
			return { error::none };
		}
		else
			return ctx.to_error(error::syntax_error);

		ctx.next();
	}
	
	return ctx.to_error(error::unexpected_end);
}

//---------------------------------------------------------------------------------------------------------------------
inline error document::parse_identifier(context& ctx, size_t& result)
{
	result = _stringBuffer.size();

	char firstCh = ctx.peek();
	bool isString = (firstCh == '\'') || (firstCh == '"');

	if (isString)
	{
		ctx.next(); // Consume '\'' or '"'

		char ch = ctx.peek();
		if (!isalpha(ch) && ch != '_')
			return ctx.to_error(error::syntax_error);
	}

	while (!ctx.eof())
	{
		_stringBuffer.push_back(ctx.next());

		char ch = ctx.peek();
		if (!isalpha(ch) && !isdigit(ch) && ch != '_')
			break;
	}

	if (isString)
	{
		if (firstCh != ctx.next()) // Consume '\'' or '"'
			return ctx.to_error(error::syntax_error);
	}

	_stringBuffer.push_back(0);
	return { error::none };
}

} // namespace json5
