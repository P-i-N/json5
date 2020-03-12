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
class object;
class array;
class document;

enum class value_type : size_t { null, boolean, string, number, object, array };

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class value
{
public:
	value() noexcept = default;
	value(bool boolean) noexcept : _type(value_type::boolean), _boolean(boolean) { }
	value(double number) noexcept : _type(value_type::number), _number(number) { }
	value(const document& doc, unsigned off) noexcept : _type(value_type::string), _doc(&doc) { }

	value(value&& rValue);
	~value();

	value_type type() const noexcept { return _type; }

	bool is_null() const noexcept { return _type == value_type::null; }
	bool is_number() const noexcept { return _type == value_type::number; }
	bool is_boolean() const noexcept { return _type == value_type::boolean; }
	bool is_string() const noexcept { return _type == value_type::string; }
	bool is_object() const noexcept { return _type == value_type::object; }
	bool is_array() const noexcept { return _type == value_type::array; }

	bool get_bool(bool defaultValue = false) const noexcept;
	int get_int(int defaultValue = 0) const noexcept;
	float get_float(float defaultValue = 0.0f) const noexcept;
	double get_double(double defaultValue = 0.0) const noexcept;
	const char* get_c_str(const char* defaultValue = "") const noexcept;

	const object& get_object() const noexcept;
	const array& get_array() const noexcept;

	static const value& null_instance()
	{
		static value nullInstance;
		return nullInstance;
	}

protected:
	value(const document& doc, value_type type);

	const char* c_str_offset(size_t offset) const noexcept;

	using properties = std::unordered_map<detail::hashed_string_ref, value>;
	using values = std::vector<value>;

	union
	{
		value_type _type = value_type::null;
		const document* _doc;
	};

	union
	{
		bool _boolean;
		double _number;
		size_t _offset;
		properties* _properties;
		values* _values;
	};

	friend class document;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class object final : public value
{
public:
	object(const document& doc): value(doc, value_type::object) { }

	class iterator final
	{
	public:
		iterator(properties::const_iterator iter, const char *strBuff): _iter(iter), _stringBuffer(strBuff) { }
		bool operator==(const iterator& other) const noexcept { return _iter == other._iter; }
		iterator& operator++() { ++_iter; return *this; }

		std::pair<const char *, const value&> operator*() const
		{
			return { _stringBuffer + _iter->first.offset, _iter->second };
		}

	private:
		properties::const_iterator _iter;
		const char* _stringBuffer;
	};

	iterator begin() const noexcept;
	iterator end() const noexcept;
	iterator find(std::string_view key) const noexcept;
	bool empty() const noexcept { return _properties->empty(); }
	bool contains(std::string_view key) const noexcept;

	static const object& empty_instance()
	{
		static object emptyInstance;
		return emptyInstance;
	}

private:
	object() = default;
	friend class document;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class array final : public value
{
public:
	array(const document& doc) : value(doc, value_type::array) { }

	size_t size() const noexcept { return _values->size(); }
	bool empty() const noexcept { return _values->empty(); }
	const value& operator[](size_t index) const { return (*_values)[index]; }

	static const array& empty_instance()
	{
		static array emptyInstance;
		return emptyInstance;
	}

private:
	array() = default;
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
		invalid_literal,
		comma_expected,
		boolean_expected,
		number_expected,
		string_expected,
		object_expected,
		array_expected
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

	const value& root() const noexcept { return _root ? (*_root) : value::null_instance(); }

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

		error make_error(int type) const noexcept { return error{ type, line, column }; }
	};

	error parse(context& ctx);
	error parse(context& ctx, std::unique_ptr<value>& result);

	error parse(context& ctx, object& result);
	error parse(context& ctx, array& result);

	enum class token_type
	{
		unknown, identifier, literal, string, number, colon, comma,
		object_begin, object_end, array_begin, array_end,
		literal_true, literal_false, literal_null
	};

	error peek_next_token(context& ctx, token_type& result);

	error parse_number(context& ctx, double& result);
	error parse_string(context& ctx, unsigned& result);
	error parse_identifier(context& ctx, unsigned& result);
	error parse_literal(context& ctx, token_type& result);

	std::vector<char> _stringBuffer;

	std::unique_ptr<value> _root;

	friend class value;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline value::value(const document& doc, value_type type) : _doc(&doc), _type(type)
{
	if (_type == value_type::object)
		_properties = new properties();
	else if (_type == value_type::array)
		_values = new values();
}

//---------------------------------------------------------------------------------------------------------------------
inline value::value(value&& rValue)
{

}

//---------------------------------------------------------------------------------------------------------------------
inline value::~value()
{
	if (_type == value_type::object)
		delete _properties;
	else if (_type == value_type::array)
		delete _values;
}

//---------------------------------------------------------------------------------------------------------------------
inline bool value::get_bool(bool defaultValue) const noexcept
{
	return is_boolean() ? _boolean : defaultValue;
}

//---------------------------------------------------------------------------------------------------------------------
inline int value::get_int(int defaultValue) const noexcept
{
	return is_number() ? static_cast<int>(_number) : defaultValue;
}

//---------------------------------------------------------------------------------------------------------------------
inline float value::get_float(float defaultValue) const noexcept
{
	return is_number() ? static_cast<float>(_number) : defaultValue;
}

//---------------------------------------------------------------------------------------------------------------------
inline double value::get_double(double defaultValue) const noexcept
{
	return is_number() ? _number : defaultValue;
}

//---------------------------------------------------------------------------------------------------------------------
inline const char* value::get_c_str(const char* defaultValue) const noexcept
{
	return is_string() ? c_str_offset(_offset) : defaultValue;
}

//---------------------------------------------------------------------------------------------------------------------
inline const object& value::get_object() const noexcept
{
	return is_object() ? static_cast<const object&>(*this) : object::empty_instance();
}

//---------------------------------------------------------------------------------------------------------------------
inline const array& value::get_array() const noexcept
{
	return is_array() ? static_cast<const array&>(*this) : array::empty_instance();
}

//---------------------------------------------------------------------------------------------------------------------
inline const char* value::c_str_offset(size_t offset) const noexcept
{
	return _doc->_stringBuffer.data() + offset;
}

//---------------------------------------------------------------------------------------------------------------------
inline object::iterator object::begin() const noexcept
{
	return iterator(_properties->begin(), c_str_offset(0));
}

//---------------------------------------------------------------------------------------------------------------------
inline object::iterator object::end() const noexcept
{
	return iterator(_properties->end(), c_str_offset(0));
}

//---------------------------------------------------------------------------------------------------------------------
inline object::iterator object::find(std::string_view key) const noexcept
{
	auto hash = std::hash<std::string_view>()(key);
	return iterator(_properties->find({ hash }), c_str_offset(0));
}

//---------------------------------------------------------------------------------------------------------------------
inline bool object::contains(std::string_view key) const noexcept
{
	auto hash = std::hash<std::string_view>()(key);
	return _properties->contains({ hash });
}

//---------------------------------------------------------------------------------------------------------------------
inline document::document()
{

}

//---------------------------------------------------------------------------------------------------------------------
inline error document::parse(context& ctx)
{
	_root.reset();

	_stringBuffer.reserve(8192); // Reserve 8kB for strings
	_stringBuffer.clear();
	_stringBuffer.push_back(0);

	token_type tt = token_type::unknown;
	if (auto err = peek_next_token(ctx, tt))
		return err;
	
	switch (tt)
	{
		case token_type::array_begin:
		{
			auto rootArray = std::make_unique<array>(*this);
			if (auto err = parse(ctx, *rootArray))
				return err;

			_root = std::move(rootArray);
		}
		break;

		case token_type::object_begin:
		{
			auto rootObject = std::make_unique<object>(*this);
			if (auto err = parse(ctx, *rootObject))
				return err;

			_root = std::move(rootObject);
		}
		break;

		default:
			return ctx.make_error(error::invalid_root);
	}
	
	return { error::none };
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
			if (double number = 0.0; auto err = parse_number(ctx, number))
				return err;
			else
				result = std::make_unique<value>(number);
		}
		break;

		case token_type::string:
		{
			if (unsigned offset = 0; auto err = parse_string(ctx, offset))
				return err;
			else
				result = std::make_unique<value>(*this, offset);
		}
		break;

		case token_type::identifier:
		{
			if (token_type lit = token_type::unknown; auto err = parse_literal(ctx, lit))
				return err;
			else
			{
				if (lit == token_type::literal_true)
					result = std::make_unique<value>(true);
				else if (lit == token_type::literal_false)
					result = std::make_unique<value>(false);
				else if (lit == token_type::literal_null)
					result = std::make_unique<value>();
				else
					return ctx.make_error(error::invalid_literal);
			}
		}
		break;

		case token_type::object_begin:
		{
			if (auto objectResult = std::make_unique<object>(*this); auto err = parse(ctx, *objectResult))
				return err;
			else
				result = std::move(objectResult);
		}
		break;

		case token_type::array_begin:
		{
			
			if (auto arrayResult = std::make_unique<array>(*this); auto err = parse(ctx, *arrayResult))
				return err;
			else
				result = std::move(arrayResult);
		}
		break;

		default:
			return ctx.make_error(error::syntax_error);
	}

	return { error::none };
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

		unsigned keyOffset;

		switch (tt)
		{
			case token_type::identifier:
			case token_type::string:
			{
				if (expectComma)
					return ctx.make_error(error::comma_expected);

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
					return ctx.make_error(error::syntax_error);

				ctx.next(); // Consume ','
				expectComma = false;
				continue;
			}

			default:
				return expectComma ? ctx.make_error(error::comma_expected) : ctx.make_error(error::syntax_error);
		}
		
		if (auto err = peek_next_token(ctx, tt))
			return err;

		if (tt != token_type::colon)
			return ctx.make_error(error::syntax_error);

		ctx.next(); // Consume ':'

		std::unique_ptr<value> valuePtr;
		if (auto err = parse(ctx, valuePtr))
			return err;

		detail::hashed_string_ref hashedKey;
		hashedKey.offset = keyOffset;

		auto sv = std::string_view(_stringBuffer.data() + hashedKey.offset);
		hashedKey.hash = std::hash<std::string_view>()(sv);

		result._properties->insert({ hashedKey, std::move(valuePtr) });
		expectComma = true;
	}

	return ctx.make_error(error::unexpected_end);
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
				return ctx.make_error(error::comma_expected);

			ctx.next(); // Consume ','
			continue;
		}

		std::unique_ptr<value> valuePtr;
		if (auto err = parse(ctx, valuePtr))
			return err;

		result._values.push_back(std::move(valuePtr));
		expectComma = true;
	}

	return ctx.make_error(error::unexpected_end);
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
				return ctx.make_error(error::syntax_error);

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
			return ctx.make_error(error::syntax_error);

		ctx.next();
	}
	
	return ctx.make_error(error::unexpected_end);
}

//---------------------------------------------------------------------------------------------------------------------
inline error document::parse_number(context& ctx, double& result)
{
	size_t offset = _stringBuffer.size();
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
		_stringBuffer.data() + offset,
		_stringBuffer.data() + offset + length,
		result);

	if (convResult.ec != std::errc())
		return ctx.make_error(error::syntax_error);

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error document::parse_string(context& ctx, unsigned& result)
{
	bool singleQuoted = ctx.peek() == '\'';
	ctx.next(); // Consume '\'' or '"'

	result = static_cast<unsigned>(_stringBuffer.size());

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
		return ctx.make_error(error::unexpected_end);

	_stringBuffer.push_back(0);
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error document::parse_identifier(context& ctx, unsigned& result)
{
	result = static_cast<unsigned>(_stringBuffer.size());

	char firstCh = ctx.peek();
	bool isString = (firstCh == '\'') || (firstCh == '"');

	if (isString)
	{
		ctx.next(); // Consume '\'' or '"'

		char ch = ctx.peek();
		if (!isalpha(ch) && ch != '_')
			return ctx.make_error(error::syntax_error);
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
			return ctx.make_error(error::syntax_error);
	}

	_stringBuffer.push_back(0);
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error document::parse_literal(context& ctx, token_type& result)
{
	char ch = ctx.peek();

	// "true"
	if (ch == 't')
	{
		ctx.next(); // Consume 't'

		if (ctx.next() == 'r' && ctx.next() == 'u' && ctx.next() == 'e')
		{
			result = token_type::literal_true;
			return { error::none };
		}
	}
	// "false"
	else if (ch == 'f')
	{
		ctx.next(); // Consume 'f'

		if (ctx.next() == 'a' && ctx.next() == 'l' && ctx.next() == 's' && ctx.next() == 'e')
		{
			result = token_type::literal_false;
			return { error::none };
		}
	}
	// "null"
	else if (ch == 'n')
	{
		ctx.next(); // Consume 'n'

		if (ctx.next() == 'u' && ctx.next() == 'l' && ctx.next() == 'l')
		{
			result = token_type::literal_null;
			return { error::none };
		}
	}

	return ctx.make_error(error::invalid_literal);
}

} // namespace json5
