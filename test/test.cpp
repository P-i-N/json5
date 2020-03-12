#include <json5/json5.hpp>
#include <json5/json5_reflect.hpp>

#include <iostream>

//---------------------------------------------------------------------------------------------------------------------
void PrintJSONValue(const json5::value& value, int depth = 0)
{
	if (value.is_null())
		std::cout << "null";
	else if (value.is_boolean())
		std::cout << value.get_bool();
	else if (value.is_number())
		std::cout << value.get_int();
	else if (value.is_string())
		std::cout << "\"" << value.get_c_str() << "\"";
	else if (value.is_array())
	{
		std::cout << "[" << std::endl;

		auto array = value.get_array();
		for (size_t i = 0, S = array.size(); i < S; ++i)
		{
			for (int i = 0; i <= depth; ++i) std::cout << "  ";
			PrintJSONValue(array[i], depth + 1);
			if (i < S - 1) std::cout << ",";
			std::cout << std::endl;
		}

		for (int i = 0; i < depth; ++i) std::cout << "  ";
		std::cout << "]";
	}
	else if (value.is_object())
	{
		std::cout << "{" << std::endl;

		auto object = value.get_object();
		size_t count = object.size();
		for (auto kvp : value.get_object())
		{
			for (int i = 0; i <= depth; ++i) std::cout << "  ";
			std::cout << kvp.first << ": ";
			PrintJSONValue(kvp.second, depth + 1);
			if (--count) std::cout << ",";
			std::cout << std::endl;
		}

		for (int i = 0; i < depth; ++i) std::cout << "  ";
		std::cout << "}";
	}

	if (!depth)
		std::cout << std::endl;
}

//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	int sz = sizeof(json5::value);
	int objSize = sizeof(json5::object);

	json5::document doc;

	if (auto err = doc.parse("{ id: null, // Comment!\narr: [ 1, 2, 3, 4, 5 ] }"))
	{
		printf("Error at line %d, column %d!\n", err.line, err.column);
	}

	PrintJSONValue(doc.root());

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
