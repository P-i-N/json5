#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace json5::detail {

struct string_ref
{
	size_t offset;
	size_t length;
};

struct hashed_string_ref : string_ref
{
	size_t hash;
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
	value(document& doc, value_type type): _doc(doc), _type(type) { }

	value_type type() const noexcept { return _type; }

protected:
	document& _doc;
	value_type _type = value_type::null;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class number final : public value
{
public:
	number(document& doc): value(doc, value_type::number) { }

private:

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class object final : public value
{
public:
	object(document& doc): value(doc, value_type::object) { }

private:
	std::unordered_map<detail::hashed_string_ref, std::unique_ptr<value>> _properties;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class array final : public value
{
public:
	array(document& doc) : value(doc, value_type::array) { }

private:
	std::vector<std::unique_ptr<value>> _values;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class document final
{
public:
	document();

	bool parse(std::string_view str) noexcept;

private:
	std::vector<char> _stringBuffer;

	object _root;

	friend class value;
	friend class number;
	friend class string;
	friend class object;
	friend class array;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
document::document()
	: _root(*this)
{

}

//---------------------------------------------------------------------------------------------------------------------
bool document::parse(std::string_view str) noexcept
{
	return true;
}

} // namespace json5
