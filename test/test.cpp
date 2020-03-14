#include <json5/json5.hpp>
#include <json5/json5_reflect.hpp>

#include <chrono>
#include <iostream>

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
void PrintJSONValue(const json5::value& value, int depth = 0)
{
	json5::to_stream(std::cout, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool PrintError(const json5::error& err)
{
	const char* errStr = "";

	switch (err.type)
	{
		case json5::error::none: return false;
		case json5::error::invalid_root: errStr = "invalid root"; break;
		case json5::error::unexpected_end: errStr = "unexpected end"; break;
		case json5::error::syntax_error: errStr = "syntax error"; break;
		case json5::error::invalid_literal: errStr = "invalid literal"; break;
		case json5::error::invalid_escape_seq: errStr = "invalid escape sequence"; break;
		case json5::error::comma_expected: errStr = "comma expected"; break;
		case json5::error::colon_expected: errStr = "colon expected"; break;
		case json5::error::boolean_expected: errStr = "boolean expected"; break;
		case json5::error::number_expected: errStr = "number expected"; break;
		case json5::error::string_expected: errStr = "string expected"; break;
		case json5::error::object_expected: errStr = "object expected"; break;
		case json5::error::array_expected: errStr = "array expected"; break;
	}

	printf("Error at line %d, column %d: %s\n", err.line, err.column, errStr);
	return true;
}

//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	/// Load from file
	{
		json5::document doc;
		PrintError(json5::from_file("short_example.json5", doc));

		PrintJSONValue(doc.root());
	}

	/// File load/save test
	{
		json5::document doc1;
		json5::document doc2;
		{
			Stopwatch sw{ "Load twitter.json" };
			PrintError(json5::from_file("twitter.json", doc1));
		}

		{
			Stopwatch sw{ "Save twitter.json5" };
			json5::to_file("twitter.json5", doc1);
		}

		{
			Stopwatch sw{ "Reload twitter.json5" };
			json5::from_file("twitter.json5", doc2);
		}

		if (doc1 == doc2)
			std::cout << "doc1 == doc2" << std::endl;
		else
			std::cout << "doc1 != doc2" << std::endl;
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

		PrintJSONValue(doc.root());
	}

	struct Foo
	{
		int x = 0;
		int y = 0;
		int z = 0;

		JSON5_REFLECT(x, y, z)
	};

	Foo f;
	f.x = 123;
	f.y = 456;
	f.z = 789;

	std::cout << json5::to_string(f) << std::endl;

	Foo foo;
	json5::from_string("{ x: 1, y: 2, z: 3 }", foo);

	return 0;
}
