# json5
**`json5`** is a small header only C++ library for parsing [JSON](https://en.wikipedia.org/wiki/JSON) or [**JSON5**](https://json5.org/) data. It also comes with a simple reflection system for easy serialization and deserialization of C++ structs.

## Example
```cpp
#include <json5/json5.hpp>

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
This is the root data structure for all library operations. It is an **immutable** container and holds finished parsed representation of your JSON data. You can initialize `json5::document` either by using `json5::builder` (see below) or by using `json5::from_`* functions.

### Building

### `json5::builder`
**TBD**

### Filtering
**`json5::document`** 

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

### `json5::object_view`

### `json5::array_view`

## Reflection API

### `JSON5_REFLECT(...)`
### `JSON5_ENUM(...)`
**TBD**

### Basic supported types
- `bool`
- `int`, `float`, `double`
- `std::string`
- `std::vector`, `std::map`, `std::unordered_map`, `std::array`
- `C array`

## Additional examples
- [Loading documents](#loading-documents)
	- [Read from file](#read-from-file)
	- [Read from string](#read-from-string)
	- [Read from stream](#read-from-stream)
- [Saving documents](#saving-documents)
	- [Write to file](#write-to-file)
	- [Write to string](#write-to-string)
	- [Write to stream](#write-to-stream)
- [Reflection](#reflection)
	- [Serialize custom type](#serialize-custom-type)
	- [Serialize enum](#serialize-enum)

### Loading documents

##### Read from file
```cpp
json5::document doc;
if (auto err = json5::from_file("myfile.json", doc))
	std::cerr << json5::to_string(err) << std::endl;
```

##### Read from string
```cpp
json5::document doc;
if (auto err = json5::from_string("{ x: 1, y: true, z: 'Hello!' }", doc))
	std::cerr << json5::to_string(err) << std::endl;
```

##### Read from stream
```cpp
std::istream myInputStream...;

json5::document doc;
if (auto err = json5::from_stream(myInputStream, doc))
	std::cerr << json5::to_string(err) << std::endl;
```

### Saving documents

##### Write to file

##### Write to string

##### Write to stream

### Reflection

##### Serialize custom type
```cpp
// Let's have a 3D vector struct:
struct vec3 { float x, y, z; };

// Let's have a triangle struct with 'vec3' members
struct Triangle
{
	vec3 a, b, c;

	JSON5_REFLECT(a, b, c)
};

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

##### Serialize enum
```cpp
enum class MyEnum
{
	Zero,
	First,
	Second,
	Third
};

// (must be placed in global namespce, requires C++20)
JSON5_ENUM( MyEnum, Zero, First, Second, Third )
```
