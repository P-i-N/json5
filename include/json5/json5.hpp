#pragma once

#include <charconv>
#include <cstdint>
#include <fstream>
#include <istream>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace json5::detail {

struct reader;

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

enum class value_type : size_t { null = 0, boolean, number, string, object, array };

class value final
{
	static constexpr size_t size_t_msbit = (static_cast<size_t>(1) << (sizeof(size_t) * 8 - 1));

public:
	value() noexcept = default;

	value_type type() const noexcept;

	bool is_null() const noexcept { return type() == value_type::null; }
	bool is_boolean() const noexcept { return type() == value_type::boolean; }
	bool is_number() const noexcept { return type() == value_type::number; }
	bool is_string() const noexcept { return type() == value_type::string; }
	bool is_object() const noexcept { return type() == value_type::object; }
	bool is_array() const noexcept { return type() == value_type::array; }
	bool is_integer() const noexcept { double _; return is_number() && (std::modf(_number, &_) == 0.0); }

	bool get_bool(bool val = false) const noexcept { return is_boolean() ? _boolean : val; }
	int get_int(int val = 0) const noexcept { return is_number() ? static_cast<int>(_number) : val; }
	int64_t get_int64(int64_t val = 0) const noexcept { return is_number() ? static_cast<int64_t>(_number) : val; }
	unsigned get_uint(unsigned val = 0) const noexcept { return is_number() ? static_cast<unsigned>(_number) : val; }
	uint64_t get_uint64(int64_t val = 0) const noexcept { return is_number() ? static_cast<uint64_t>(_number) : val; }
	float get_float(float val = 0.0f) const noexcept { return is_number() ? static_cast<float>(_number) : val; }
	double get_double(double val = 0.0) const noexcept { return is_number() ? _number : val; }
	const char* get_c_str(const char* val = "") const noexcept;

	bool operator==(const value& other) const noexcept;
	bool operator!=(const value& other) const noexcept { return !((*this) == other); }

private:
	using properties_t = std::unordered_map<detail::hashed_string_ref, value>;
	using values_t = std::vector<value>;
	using pointer_map_t = std::unordered_map<size_t, size_t>;

	value(bool val) noexcept : _type(value_type::boolean), _boolean(val) { }
	value(double val) noexcept : _type(value_type::number), _number(val) { }
	value(const class document* doc, unsigned offset) noexcept : _doc(doc), _offset(offset | size_t_msbit) { }
	value(const class document* doc, properties_t& props) noexcept : _doc(doc), _properties(&props) { }
	value(const class document* doc, values_t& vals) : _type(value_type::array), _values(&vals) { }

	void relink(const class document* newDoc, pointer_map_t* ptrMap = nullptr) noexcept;

	static const value& empty_object()
	{
		static properties_t emptyProperties;
		static value emptyObject(nullptr, emptyProperties);
		return emptyObject;
	}

	static const value& empty_array()
	{
		static values_t emptyValues;
		static value emptyArray(nullptr, emptyValues);
		return emptyArray;
	}

	union
	{
		value_type _type = value_type::null;
		const class document* _doc;
	};

	union
	{
		bool _boolean;
		double _number;
		size_t _offset;
		properties_t* _properties;
		values_t* _values;
	};

	friend class document;
	friend class object;
	friend class array;
	friend detail::reader;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class object final
{
public:
	object(const value& v) : _value(v.is_object() ? v : value::empty_object()) { }

	class iterator final
	{
	public:
		iterator(value::properties_t::const_iterator iter, const char *strBuff): _iter(iter), _stringBuffer(strBuff) { }
		bool operator==(const iterator& other) const noexcept { return _iter == other._iter; }
		iterator& operator++() { ++_iter; return *this; }
		auto operator*() const { return std::pair(_stringBuffer + _iter->first.offset, _iter->second); }

	private:
		value::properties_t::const_iterator _iter;
		const char* _stringBuffer;
	};

	iterator begin() const noexcept;
	iterator end() const noexcept;
	iterator find(std::string_view key) const noexcept;
	size_t size() const noexcept { return _value._properties->size(); }
	bool empty() const noexcept { return _value._properties->empty(); }
	bool contains(std::string_view key) const noexcept;

private:
	const value& _value;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class array final
{
public:
	array(const value& v) : _value(v.is_array() ? v : value::empty_array()) { }

	using iterator = value::values_t::const_iterator;

	iterator begin() const noexcept { return _value._values->begin(); }
	iterator end() const noexcept { return _value._values->end(); }
	size_t size() const noexcept { return _value._values->size(); }
	bool empty() const noexcept { return _value._values->empty(); }
	const value& operator[](size_t index) const { return (*_value._values)[index]; }

private:
	const value& _value;
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
		invalid_escape_seq,
		comma_expected,
		colon_expected,
		boolean_expected,
		number_expected,
		string_expected,
		object_expected,
		array_expected
	};

	int type = none;
	int line = 0;
	int column = 0;

	operator int() const noexcept { return type; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class document final
{
public:
	document() = default;
	document(const document& copy) { assign_copy(copy); }
	document(document&& rValue) noexcept { assign_rvalue(std::forward<document>(rValue)); }

	document& operator=(const document& copy) { assign_copy(copy); return *this; }
	document& operator=(document&& rValue) noexcept { assign_rvalue(std::forward<document>(rValue)); return *this; }

	bool operator==(const document& other) const noexcept { return _root == other._root; }
	bool operator!=(const document& other) const noexcept { return !((*this) == other); }

	const value& root() const noexcept { return _root; }
	
private:
	void assign_copy(const document& copy);
	void assign_rvalue(document&& rValue) noexcept;

	std::string _stringBuffer;
	std::vector<std::unique_ptr<value::properties_t>> _propertiesBuffer;
	std::vector<std::unique_ptr<value::values_t>> _valuesBuffer;

	value make_object() { return value(this, *_propertiesBuffer.emplace_back(new value::properties_t())); }
	value make_array() { return value(this, *_valuesBuffer.emplace_back(new value::values_t())); }

	value _root;

	friend value;
	friend object;
	friend detail::reader;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace detail {

enum class token_type
{
	unknown, identifier, string, number, colon, comma,
	object_begin, object_end, array_begin, array_end,
	literal_true, literal_false, literal_null
};

struct reader
{
	document& doc;
	std::istream& is;

	int line = 1;
	int column = 1;

	char next();
	char peek() { return is.peek(); }
	bool eof() const { return is.eof(); }
	error make_error(int type) const noexcept { return error{ type, line, column }; }

	error parse();
	error parse_value(value& result);
	error parse_properties(value::properties_t& result);
	error parse_values(value::values_t& result);
	error peek_next_token(token_type& result);
	error parse_number(double& result);
	error parse_string(unsigned& result);
	error parse_identifier(unsigned& result);
	error parse_literal(token_type& result);
};

//---------------------------------------------------------------------------------------------------------------------
inline char reader::next()
{
	if (is.peek() == '\n')
	{
		column = 0;
		++line;
	}

	++column;
	return is.get();
}

//---------------------------------------------------------------------------------------------------------------------
inline error reader::parse()
{
	doc._root = value();

	doc._stringBuffer.clear();
	doc._stringBuffer.push_back(0);

	token_type tt = token_type::unknown;
	if (auto err = peek_next_token(tt))
		return err;

	switch (tt)
	{
		case token_type::array_begin:
		{
			doc._root = doc.make_array();
			if (auto err = parse_values(*doc._root._values))
				return err;
		}
		break;

		case token_type::object_begin:
		{
			doc._root = doc.make_object();
			if (auto err = parse_properties(*doc._root._properties))
				return err;
		}
		break;

		default:
			return make_error(error::invalid_root);
	}

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error reader::parse_value(value& result)
{
	token_type tt = token_type::unknown;
	if (auto err = peek_next_token(tt))
		return err;

	switch (tt)
	{
		case token_type::number:
		{
			if (double number = 0.0; auto err = parse_number(number))
				return err;
			else
				result = value(number);
		}
		break;

		case token_type::string:
		{
			if (unsigned offset = 0; auto err = parse_string(offset))
				return err;
			else
				result = value(&doc, offset);
		}
		break;

		case token_type::identifier:
		{
			if (token_type lit = token_type::unknown; auto err = parse_literal(lit))
				return err;
			else
			{
				if (lit == token_type::literal_true)
					result = value(true);
				else if (lit == token_type::literal_false)
					result = value(false);
				else if (lit == token_type::literal_null)
					result = value();
				else
					return make_error(error::invalid_literal);
			}
		}
		break;

		case token_type::object_begin:
		{
			result = doc.make_object();
			if (auto err = parse_properties(*result._properties))
				return err;
		}
		break;

		case token_type::array_begin:
		{
			result = doc.make_array();
			if (auto err = parse_values(*result._values))
				return err;
		}
		break;

		default:
			return make_error(error::syntax_error);
	}

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error reader::parse_properties(value::properties_t& result)
{
	next(); // Consume '{'

	bool expectComma = false;
	while (!eof())
	{
		token_type tt = token_type::unknown;
		if (auto err = peek_next_token(tt))
			return err;

		unsigned keyOffset;

		switch (tt)
		{
		case token_type::identifier:
		case token_type::string:
		{
			if (expectComma)
				return make_error(error::comma_expected);

			if (auto err = parse_identifier(keyOffset))
				return err;
		}
		break;

		case token_type::object_end:
			next(); // Consume '}'
			return { error::none };

		case token_type::comma:
			if (!expectComma)
				return make_error(error::syntax_error);

			next(); // Consume ','
			expectComma = false;
			continue;

		default:
			return expectComma ? make_error(error::comma_expected) : make_error(error::syntax_error);
		}

		if (auto err = peek_next_token(tt))
			return err;

		if (tt != token_type::colon)
			return make_error(error::colon_expected);

		next(); // Consume ':'

		value newValue;
		if (auto err = parse_value(newValue))
			return err;

		detail::hashed_string_ref hashedKey;
		hashedKey.offset = keyOffset;

		auto sv = std::string_view(doc._stringBuffer.data() + hashedKey.offset);
		hashedKey.hash = std::hash<std::string_view>()(sv);

		result.insert({ hashedKey, newValue });
		expectComma = true;
	}

	return make_error(error::unexpected_end);
}

//---------------------------------------------------------------------------------------------------------------------
inline error reader::parse_values(value::values_t& result)
{
	next(); // Consume '['

	bool expectComma = false;
	while (!eof())
	{
		token_type tt = token_type::unknown;
		if (auto err = peek_next_token(tt))
			return err;

		if (tt == token_type::array_end)
		{
			next(); // Consume ']'
			return { error::none };
		}
		else if (expectComma)
		{
			expectComma = false;

			if (tt != token_type::comma)
				return make_error(error::comma_expected);

			next(); // Consume ','
			continue;
		}

		if (auto err = parse_value(result.emplace_back()))
			return err;

		expectComma = true;
	}

	return make_error(error::unexpected_end);
}

//---------------------------------------------------------------------------------------------------------------------
inline error reader::peek_next_token(token_type& result)
{
	enum class comment_type { none, line, block } parsingComment = comment_type::none;

	while (!eof())
	{
		char ch = peek();
		if (ch == '\n')
		{
			if (parsingComment == comment_type::line)
				parsingComment = comment_type::none;
		}
		else if (parsingComment != comment_type::none || ch <= 32)
		{
			if (parsingComment == comment_type::block && ch == '*')
			{
				next(); // Consume '*'

				if (peek() == '/')
					parsingComment = comment_type::none;
			}
		}
		else if (ch == '/')
		{
			next(); // Consume '/'

			if (peek() == '/')
				parsingComment = comment_type::line;
			else if (peek() == '*')
				parsingComment = comment_type::block;
			else
				return make_error(error::syntax_error);
		}
		else if (strchr("{}[]:,", ch))
		{
			if (ch == '{')
				result = token_type::object_begin;
			else if (ch == '}')
				result = token_type::object_end;
			else if (ch == '[')
				result = token_type::array_begin;
			else if (ch == ']')
				result = token_type::array_end;
			else if (ch == ':')
				result = token_type::colon;
			else if (ch == ',')
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
			if (ch == '+') next(); // Consume leading '+'

			result = token_type::number;
			return { error::none };
		}
		else if (ch == '"' || ch == '\'')
		{
			result = token_type::string;
			return { error::none };
		}
		else
			return make_error(error::syntax_error);

		next();
	}

	return make_error(error::unexpected_end);
}

//---------------------------------------------------------------------------------------------------------------------
inline error reader::parse_number(double& result)
{
	auto& strBuffer = doc._stringBuffer;
	size_t offset = strBuffer.size();
	size_t length = 0;

	while (!eof())
	{
		strBuffer.push_back(next());
		++length;

		char ch = peek();
		if (ch <= 32 || ch == ',' || ch == '}' || ch == ']')
			break;
	}

	strBuffer.push_back(0);

	auto convResult = std::from_chars(
		strBuffer.data() + offset,
		strBuffer.data() + offset + length,
		result);

	if (convResult.ec != std::errc())
		return make_error(error::syntax_error);

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error reader::parse_string(unsigned& result)
{
	bool singleQuoted = peek() == '\'';
	next(); // Consume '\'' or '"'

	auto& strBuffer = doc._stringBuffer;
	result = static_cast<unsigned>(strBuffer.size());

	while (!eof())
	{
		char ch = peek();
		if ((singleQuoted && ch == '\'') || (!singleQuoted && ch == '"'))
		{
			next(); // Consume '\'' or '"'
			break;
		}
		else if (ch == '\\')
		{
			next(); // Consume '\\'

			ch = peek();
			if (ch == '\n')
				next();
			else if (ch == 't' && next())
				strBuffer.push_back('\t');
			else if (ch == 'n' && next())
				strBuffer.push_back('\n');
			else if (ch == 'r' && next())
				strBuffer.push_back('\r');
			else if (ch == '\\' && next())
				strBuffer.push_back('\\');
			else if (ch == '\'' && next())
				strBuffer.push_back('\'');
			else if (ch == '"' && next())
				strBuffer.push_back('"');
			else if (ch == '\\' && next())
				strBuffer.push_back('\\');
			else
				return make_error(error::invalid_escape_seq);
		}
		else
			strBuffer.push_back(next());
	}

	if (eof())
		return make_error(error::unexpected_end);

	strBuffer.push_back(0);
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error reader::parse_identifier(unsigned& result)
{
	auto& strBuffer = doc._stringBuffer;
	result = static_cast<unsigned>(strBuffer.size());

	char firstCh = peek();
	bool isString = (firstCh == '\'') || (firstCh == '"');

	if (isString)
	{
		next(); // Consume '\'' or '"'

		char ch = peek();
		if (!isalpha(ch) && ch != '_')
			return make_error(error::syntax_error);
	}

	while (!eof())
	{
		strBuffer.push_back(next());

		char ch = peek();
		if (!isalpha(ch) && !isdigit(ch) && ch != '_')
			break;
	}

	if (isString && firstCh != next()) // Consume '\'' or '"'
		return make_error(error::syntax_error);

	strBuffer.push_back(0);
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
inline error reader::parse_literal(token_type& result)
{
	char ch = peek();

	// "true"
	if (ch == 't')
	{
		if (next() && next() == 'r' && next() == 'u' && next() == 'e')
		{
			result = token_type::literal_true;
			return { error::none };
		}
	}
	// "false"
	else if (ch == 'f')
	{
		if (next() && next() == 'a' && next() == 'l' && next() == 's' && next() == 'e')
		{
			result = token_type::literal_false;
			return { error::none };
		}
	}
	// "null"
	else if (ch == 'n')
	{
		if (next() && next() == 'u' && next() == 'l' && next() == 'l')
		{
			result = token_type::literal_null;
			return { error::none };
		}
	}

	return make_error(error::invalid_literal);
}

} // namespace detail

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline value_type value::type() const noexcept
{
	if (_type > value_type::array)
		return (_offset & size_t_msbit) ? value_type::string : value_type::object;

	return _type;
}

//---------------------------------------------------------------------------------------------------------------------
inline const char* value::get_c_str(const char* defaultValue) const noexcept
{
	return is_string() ? (_doc->_stringBuffer.data() + (_offset & ~size_t_msbit)) : defaultValue;
}

//---------------------------------------------------------------------------------------------------------------------
inline bool value::operator==(const value& other) const noexcept
{
	auto t = type();
	if (t != other.type())
		return false;

	switch (t)
	{
		case value_type::boolean: return _boolean == other._boolean;
		case value_type::number:  return _number == other._number;
		case value_type::string:  return !strcmp(get_c_str(), other.get_c_str());
		case value_type::object:  return (*_properties) == (*(other._properties));
		case value_type::array:   return (*_values) == (*(other._values));
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
inline void value::relink(const class document* newDoc, pointer_map_t* ptrMap) noexcept
{
	if (_type > value_type::array)
		_doc = newDoc;

	if (ptrMap)
	{
		if (is_array())
			_values = reinterpret_cast<values_t*>((*ptrMap)[reinterpret_cast<size_t>(_values)]);
		else if (is_object())
			_properties = reinterpret_cast<properties_t*>((*ptrMap)[reinterpret_cast<size_t>(_properties)]);
	}
}

//---------------------------------------------------------------------------------------------------------------------
inline object::iterator object::begin() const noexcept
{
	return iterator(_value._properties->begin(), _value._doc->_stringBuffer.data());
}

//---------------------------------------------------------------------------------------------------------------------
inline object::iterator object::end() const noexcept
{
	return iterator(_value._properties->end(), _value._doc->_stringBuffer.data());
}

//---------------------------------------------------------------------------------------------------------------------
inline object::iterator object::find(std::string_view key) const noexcept
{
	auto hash = std::hash<std::string_view>()(key);
	return iterator(_value._properties->find({ hash }), _value._doc->_stringBuffer.data());
}

//---------------------------------------------------------------------------------------------------------------------
inline bool object::contains(std::string_view key) const noexcept
{
	auto hash = std::hash<std::string_view>()(key);
	return _value._properties->contains({ hash });
}

//---------------------------------------------------------------------------------------------------------------------
void document::assign_copy(const document& copy)
{
	_stringBuffer = copy._stringBuffer;

	std::unordered_map<size_t, size_t> ptrMapping;

	for (const auto& props : copy._propertiesBuffer)
	{
		auto& ptr = _propertiesBuffer.emplace_back(new value::properties_t(*props));
		ptrMapping.insert({ reinterpret_cast<size_t>(props.get()), reinterpret_cast<size_t>(ptr.get()) });
	}

	for (const auto& vals : copy._valuesBuffer)
	{
		auto& ptr = _valuesBuffer.emplace_back(new value::values_t(*vals));
		ptrMapping.insert({ reinterpret_cast<size_t>(vals.get()), reinterpret_cast<size_t>(ptr.get()) });
	}

	_root = copy._root;
	_root.relink(this, &ptrMapping);

	for (const auto& props : _propertiesBuffer)
		for (auto& [k, v] : *props)
			v.relink(this, &ptrMapping);

	for (const auto& vals : _valuesBuffer)
		for (auto& v : *vals)
			v.relink(this, &ptrMapping);
}

//---------------------------------------------------------------------------------------------------------------------
void document::assign_rvalue(document&& rValue) noexcept
{
	_stringBuffer = std::move(rValue._stringBuffer);
	_propertiesBuffer = std::move(rValue._propertiesBuffer);
	_valuesBuffer = std::move(rValue._valuesBuffer);
	_root = std::move(rValue._root);

	_root.relink(this);

	for (const auto& props : _propertiesBuffer)
		for (auto& [k, v] : *props)
			v.relink(this);

	for (const auto& vals : _valuesBuffer)
		for (auto& v : *vals)
			v.relink(this);
}

//---------------------------------------------------------------------------------------------------------------------
inline void to_stream(std::ostream& os, const value& v, int depth = 0)
{
	if (v.is_null())
		os << "null";
	else if (v.is_boolean())
		os << (v.get_bool() ? "true" : "false");
	else if (v.is_integer())
		os << v.get_int64();
	else if (v.is_number())
		os << v.get_double();
	else if (v.is_string())
	{
		os << "\"";

		const auto* str = v.get_c_str();
		while (*str)
		{
			if (str[0] == '\n')
				os << "\\n";
			else if (str[0] == '\r')
				os << "\\r";
			else if (str[0] == '"')
				os << "\\\"";
			else if (str[0] == '\\')
				os << "\\\\";
			else
				os << *str;

			++str;
		}

		os << "\"";
	}
	else if (v.is_array())
	{
		if (auto array = json5::array(v); !array.empty())
		{
			os << "[" << std::endl;
			for (size_t i = 0, S = array.size(); i < S; ++i)
			{
				for (int i = 0; i <= depth; ++i) os << "  ";
				to_stream(os, array[i], depth + 1);
				if (i < S - 1) os << ",";
				os << std::endl;
			}

			for (int i = 0; i < depth; ++i) os << "  ";
			os << "]";
		}
		else
			os << "[]";
	}
	else if (v.is_object())
	{
		if (auto object = json5::object(v); !object.empty())
		{
			os << "{" << std::endl;
			size_t count = object.size();
			for (auto kvp : object)
			{
				for (int i = 0; i <= depth; ++i) os << "  ";
				os << kvp.first << ": ";
				to_stream(os, kvp.second, depth + 1);
				if (--count) os << ",";
				os << std::endl;
			}

			for (int i = 0; i < depth; ++i) os << "  ";
			os << "}";
		}
		else
			os << "{}";
	}

	if (!depth)
		os << std::endl;
}

//---------------------------------------------------------------------------------------------------------------------
inline void to_stream(std::ostream& os, const document& doc) { to_stream(os, doc.root(), 0); }

//---------------------------------------------------------------------------------------------------------------------
inline bool to_file(const std::string& fileName, const document& doc)
{
	std::ofstream ofs(fileName);
	to_stream(ofs, doc);
	return true;
}

//---------------------------------------------------------------------------------------------------------------------
inline error from_stream(std::istream& is, document& doc)
{
	detail::reader r{ doc, is };
	return r.parse();
}

//---------------------------------------------------------------------------------------------------------------------
inline error from_string(const std::string& str, document& doc)
{
	std::istringstream is(str);
	return from_stream(is, doc);
}

//---------------------------------------------------------------------------------------------------------------------
inline error from_file(const std::string& fileName, document& doc)
{
	std::ifstream ifs(fileName);
	return from_stream(ifs, doc);
}

} // namespace json5
