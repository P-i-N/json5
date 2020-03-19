#include <json5/json5.hpp>

#include <chrono>
#include <iostream>
#include <map>

//---------------------------------------------------------------------------------------------------------------------
struct Stopwatch
{
	const char* name = "";
	std::chrono::nanoseconds start = std::chrono::high_resolution_clock::now().time_since_epoch();

	~Stopwatch()
	{
		auto duration = std::chrono::high_resolution_clock::now().time_since_epoch() - start;
		std::cout << name << ": " << duration.count() / 1000000 << " ms" << std::endl;
	}
};

//---------------------------------------------------------------------------------------------------------------------
bool PrintError(const json5::error& err)
{
	if (err)
	{
		std::cout << json5::to_string(err) << std::endl;
		return true;
	}

	return false;
}

// Let's have a 3D vector struct:
struct vec3 { float x, y, z; };

namespace json5::detail {

// Write vec3 as JSON array of 3 numbers
inline json5::box_value write(builder& b, const vec3& in)
{
	b.push_array();
	b += write(b, in.x);
	b += write(b, in.y);
	b += write(b, in.z);
	return b.pop();
}

// Read vec3 from JSON array
inline error read(const json5::box_value& in, vec3& out)
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


// Let's have a triangle struct with 'vec3' members
struct Triangle
{
	vec3 a, b, c;
	JSON5_REFLECT(a, b, c)
};

//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	/// Visitor test
	if (false)
	{
		json5::document doc;

		{
			Stopwatch sw{ "Load twitter.json" };
			if (auto err = json5::from_file("twitter.json", doc))
				json5::to_stream(std::cout, err);
		}
	}

	/// Build
	{
		json5::document doc;
		json5::builder b(doc);

		b.push_object();
		{
			b["x"] = b.new_string("Hello!");
			b["y"] = 123.0;
			b["z"] = true;

			b.push_array();
			{
				b += b.new_string("a");
				b += b.new_string("b");
				b += b.new_string("c");
			}
			b["arr"] = b.pop();
		}
		b.pop();

		std::cout << json5::to_string(doc);
	}

	/// Load from file
	{
		json5::document doc;
		PrintError(json5::from_file("short_example.json5", doc));
		json5::to_stream(std::cout, doc);
	}

	/// File load/save test
	{
		json5::document doc1;
		json5::document doc2;
		{
			Stopwatch sw{ "Load twitter.json (doc1)" };
			PrintError(json5::from_file("twitter.json", doc1));
		}

		{
			Stopwatch sw{ "Save twitter.json5" };
			json5::to_file("twitter.json5", doc1);
		}

		{
			Stopwatch sw{ "Reload twitter.json5 (doc2)" };
			json5::from_file("twitter.json5", doc2);
		}

		{
			Stopwatch sw{ "Compare doc1 == doc2" };
			if (doc1 == doc2)
				std::cout << "doc1 == doc2" << std::endl;
			else
				std::cout << "doc1 != doc2" << std::endl;
		}
	}

	/// Equality test
	{
		json5::document doc1;
		json5::from_string("{ x: 1, y: 2, z: 3 }", doc1);

		json5::document doc2;
		json5::from_string("{ z: 3, x: 1, y: 2 }", doc2);

		if (doc1 == doc2)
			std::cout << "doc1 == doc2" << std::endl;
		else
			std::cout << "doc1 != doc2" << std::endl;
	}

	/// String line breaks
	{
		json5::document doc;
		PrintError(json5::from_string("{ text: 'Hello\\\n, world!' }", doc));
		json5::to_stream(std::cout, doc);
	}

	/// Reflection test
	{
		struct Bar
		{
			std::string name;
			int age = 0;

			JSON5_REFLECT(name, age)
			bool operator==(const Bar& o) const noexcept { return make_tuple() == o.make_tuple(); }
		};

		struct Foo
		{
			int x = 123;
			float y = 456.0f;
			bool z = true;
			std::string text = "Hello, world!";
			std::vector<int> numbers = { 1, 2, 3, 4, 5 };
			std::map<std::string, Bar> barMap = { { "x", { "a", 1 } }, { "y", { "b", 2 } }, { "z", { "c", 3 } } };

			std::array<float, 3> position = { 10.0f, 20.0f, 30.0f };

			Bar bar = { "Somebody Unknown", 500 };

			JSON5_REFLECT(x, y, z, text, numbers, barMap, position, bar)
			bool operator==(const Foo& o) const noexcept { return make_tuple() == o.make_tuple(); }
		};

		Foo foo1;
		json5::to_file("Foo.json5", foo1);

		Foo foo2;
		json5::from_file("Foo.json5", foo2);

		if (foo1 == foo2)
			std::cout << "foo1 == foo2" << std::endl;
		else
			std::cout << "foo1 != foo2" << std::endl;
	}

	return 0;
}
