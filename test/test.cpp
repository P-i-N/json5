#include <json5/json5.hpp>

#include <iostream>

//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	json5::document doc;
	doc.parse("{ text: 'Hello, world!', arr: [ 1, 2, 3 ] }");

	if (auto err = doc.parse("{ text: 'Hello, world!', arr: [ 1, 2, 3 ] }"))
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
