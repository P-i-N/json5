# json5
**`json5`** is a small header only C++ library for parsing [JSON](https://en.wikipedia.org/wiki/JSON) or [**JSON5**](https://json5.org/) data. It also comes with a simple reflection system for easy serialization and deserialization of C++ structs.

## Example
```cpp
#include <json5/json5.hpp>
#include <json5/json5_reflect.hpp>

struct Settings
{
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	bool fullscreen = false;
	std::string renderer = "";

	JSON5_REFLECT(x, y, width, height, fullscreen, renderer)
};

Settings s;

// Fill 's' instance from file
json5::from_file("settings.json", s);

// Save 's' to file
json5::to_file("settings.json", s);
```

## Overview

### `json5::document`
This is the root data structure for all library operations. It is an **immutable** container of all values, strings, objects and holds finished parsed representation of your JSON data. You can initialize `json5::document` either by using `json5::builder` (see below) or by using `json5::from_`* functions:

**`json5::from_file`** - loads and parses JSON data from file into *`json5::document`* instance:
```cpp
json5::document doc;
if (auto err = json5::from_file("myfile.json", doc))
	std::cerr << json5::to_string(err) << std::endl;
```

**`json5::from_string`** - parses string with JSON into *`json5::document`* instance:
```cpp
json5::document doc;
if (auto err = json5::from_string("{ x: 1, y: true, z: 'Hello!' }", doc))
	std::cerr << json5::to_string(err) << std::endl;
```

**`json5::from_stream`** - parses JSON data from stream into *`json5::document`* instance
```cpp
std::istream myInputStream...;

json5::document doc;
if (auto err = json5::from_stream(myInputStream, doc))
	std::cerr << json5::to_string(err) << std::endl;
```

### Writing
**TBD**

**`json5::to_file`** - saves `json5::document` instance content into a file

**`json5::to_string`** - serializes `json5::document` instance into a string

**`json5::to_stream`** - serializes `json5::document` instance to a stream

### Building

### `json5::builder`
**TBD**

### Error reporting

Library does not rely on exceptions. Instead, most parsing functions return `json5::error` structure containing error type and location. The simplest, most convenient way to check for parsing error is to do a check within `if` condition:
```cpp
if (auto err = json5::from_file("settings.json", s))
{
	// Might print something like: invalid escape sequence at 145:8
	std::cerr << json5::to_string(err) << std::endl;
}
```

### Library data types
**TBD**

### `json5::value`

### `json5::object`

### `json5::array`

## Reflection API

### `JSON5_REFLECT(...)`
**TBD**

### Basic supported types
- `bool`
- `int`, `float`, `double`
- `std::string`
- `std::vector`, `std::map`, `std::unordered_map`, `std::array`
- `C array`

### Extending with other types
```cpp
// Let's have a 3D vector struct:
struct vec3 { float x, y, z; };

namespace json5::detail {

// Write vec3 as JSON array of 3 numbers
inline json5::value write(builder& b, const vec3 &in)
{
	b.push_array();
	b += write(b, in.x);
	b += write(b, in.y);
	b += write(b, in.z);
	return b.pop();
}

// Read vec3 from JSON array
inline error read(const json5::value& in, vec3& out)
{
	if (!in.is_array())
		return { error::array_expected };

	auto arr = json5::array(in);

	if (arr.size() != 3)
		return { error::wrong_array_size };

	if (auto err = read(arr[0], out.x))
		return err;

	if (auto err = read(arr[1], out.y))
		return err;

	if (auto err = read(arr[2], out.z))
		return err;

	return { error::none };
}

} // namespace json5::detail
```
