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
This is the main data structure for all library operations. It is an **immutable** container of all values, strings, objects and holds finished parsed representation of your JSON data.

Most parsing, reading and writing operations are done with `json5::from_`* and `json5::to_`* functions:

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
- `json5::to_file`
- `json5::to_string`
- `json5::to_stream`

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

### `json5::value`

### `json5::object`

### `json5::array`

### `json5::builder`

## Reflection API

### `JSON5_REFLECT(...)`
