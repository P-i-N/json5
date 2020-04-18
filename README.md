# `json5`
**`json5`** is a small header only C++ library for parsing [JSON](https://en.wikipedia.org/wiki/JSON) or [**JSON5**](https://json5.org/) data. It also comes with a simple reflection system for easy serialization and deserialization of C++ structs.

### Quick Example:
```cpp
#include <json5/json5_input.hpp>
#include <json5/json5_output.hpp>

struct Settings
{
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	bool fullscreen = false;
	std::string renderer = "";

	JSON5_MEMBERS(x, y, width, height, fullscreen, renderer)
};

Settings s;

// Fill 's' instance from file
json5::from_file("settings.json", s);

// Save 's' to file
json5::to_file("settings.json", s);
```

## `json5.hpp`

## `json5_input.hpp`
Provides functions to load `json5::document` from string, stream or file.

## `json5_output.hpp`
Provides functions to convert `json5::document` into string, stream or file.

## `json5_builder.hpp`

## `json5_reflect.hpp`

### Basic supported types:
- `bool`
- `int`, `float`, `double`
- `std::string`
- `std::vector`, `std::map`, `std::unordered_map`, `std::array`
- `C array`

## `json5_base.hpp`

## `json5_filter.hpp`

# FAQ



# Additional examples

### Serialize custom type:
```cpp
// Let's have a 3D vector struct:
struct vec3 { float x, y, z; };

// Let's have a triangle struct with 'vec3' members
struct Triangle
{
	vec3 a, b, c;
};

JSON5_CLASS(Triangle, a, b, c)

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

	auto arr = json5::array_view(in);

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

### Serialize enum:
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
