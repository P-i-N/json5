#include <json5/json5.hpp>
#include <json5/json5_reflect.hpp>

#include <iostream>

//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	int sz = sizeof(json5::value);
	int objSize = sizeof(json5::object);

	json5::document doc;

	if (auto err = doc.parse("{ id: null }"))
	{
		printf("Error at line %d, column %d!\n", err.line, err.column);
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
