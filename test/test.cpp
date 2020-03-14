#include <json5/json5.hpp>
#include <json5/json5_reflect.hpp>

#include <iostream>

//---------------------------------------------------------------------------------------------------------------------
void PrintJSONValue(const json5::value& value, int depth = 0)
{
	if (value.is_null())
		std::cout << "null";
	else if (value.is_boolean())
		std::cout << (value.get_bool() ? "true" : "false");
	else if (value.is_number())
		std::cout << value.get_int();
	else if (value.is_string())
		std::cout << "\"" << value.get_c_str() << "\"";
	else if (value.is_array())
	{
		std::cout << "[" << std::endl;

		auto array = json5::array(value);
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

		auto object = json5::object(value);
		size_t count = object.size();
		for (auto kvp : object)
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
	int docSize = sizeof(json5::document);

	json5::document doc;
	auto err = json5::from_string("{ id: null, arr: [ 1, 2, 3, 4, 5 ], text: 'Hello, world!', anotherObj: { x: true, y: false, z: null } }", doc);

	if (err)
	{
		printf("Error at line %d, column %d!\n", err.line, err.column);
	}

	json5::document doc2 = doc;

	PrintJSONValue(doc2.root());

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
