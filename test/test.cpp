#include <json5/json5.hpp>
#include <json5/json5_input.hpp>
#include <json5/json5_output.hpp>
#include <json5/json5_reflect.hpp>
#include <json5/json5_streams.hpp>

#include <chrono>
#include <iostream>
#include <map>
#include <type_traits>

//---------------------------------------------------------------------------------------------------------------------
struct Stopwatch
{
	const char *name = "";
	std::chrono::nanoseconds start = std::chrono::high_resolution_clock::now().time_since_epoch();

	~Stopwatch()
	{
		auto duration = std::chrono::high_resolution_clock::now().time_since_epoch() - start;
		std::cout << name << ": " << duration.count() / 1000000 << " ms" << std::endl;
	}
};

//---------------------------------------------------------------------------------------------------------------------
bool PrintError( const json5::error &err )
{
	if ( err )
	{
		std::cout << json5::error::type_string[err.type] << " at byte " << err.loc.offset << std::endl;
		return true;
	}

	return false;
}

enum class MyEnum
{
	Zero,
	First,
	Second,
	Third
};

JSON5_ENUM( MyEnum, Zero, First, Second, Third )

struct BarBase
{
	std::string name = "";
};

JSON5_CLASS( BarBase, name )

struct Bar : BarBase
{
	int age = 0;
};

JSON5_CLASS( Bar, name, age )

//---------------------------------------------------------------------------------------------------------------------
int main( int argc, char *argv[] )
{
	/// Build
	{
		json5::document doc;
		json5::builder b( doc );

		b.push_object();
		{
			b["x"] = "Hello!";
			b["y"] = 123.0f;
			b["z"] = true;

			b.push_array();
			{
				b( "a", "b", "c" );
			}
			b["arr"] = b.pop();
		}
		b.pop();

		std::cout << json5::to_string( doc );
	}

	/// Load from file
	{
		json5::document doc;
		PrintError( json5::from_file( "short_example.json5", doc ) );
		std::cout << json5::to_string( doc );

		if ( doc.is_document() )
		{
			printf( "Is document!\n" );
		}
	}

	/// File load/save test
	{
		json5::document doc1;
		json5::document doc2;
		{
			Stopwatch sw{ "Load twitter.json (doc1)" };
			PrintError( json5::from_file( "twitter.json", doc1 ) );
		}

		{
			json5::writer_params wp;
			wp.compact = true;

			Stopwatch sw{ "Save twitter.json5" };
			json5::to_file( "twitter.json5", doc1, wp );
		}

		{
			Stopwatch sw{ "Reload twitter.json5 (doc2)" };
			json5::from_file( "twitter.json5", doc2 );
		}

		{
			Stopwatch sw{ "Compare doc1 == doc2" };
			if ( doc1 == doc2 )
				std::cout << "doc1 == doc2" << std::endl;
			else
				std::cout << "doc1 != doc2" << std::endl;
		}
	}

	/// Equality test
	{
		json5::document doc1;
		json5::from_string( "{ x: 1, y: 2, z: 3 }", doc1 );

		json5::document doc2;
		json5::from_string( "{ z: 3, x: 1, y: 2 }", doc2 );

		if ( doc1 == doc2 )
			std::cout << "doc1 == doc2" << std::endl;
		else
			std::cout << "doc1 != doc2" << std::endl;
	}

	/// String line breaks
	{
		json5::document doc;
		PrintError( json5::from_string( "{ text: 'Hello\\\n, world!' }", doc ) );
		std::cout << json5::to_string( doc );
	}

	/// Reflection test
	{
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
			BarBase barBase = { "Santa Claus" };
			MyEnum e = MyEnum::Second;

			JSON5_MEMBERS( x, y, z, text, numbers, barMap, position, bar, barBase, e )
			//bool operator==( const Foo &o ) const noexcept { return make_tuple() == o.make_tuple(); }
		};

		Foo foo1;
		json5::to_file( "Foo.json5", foo1 );

		Foo foo2;
		json5::from_file( "Foo.json5", foo2 );

		/*
		if ( foo1 == foo2 )
			std::cout << "foo1 == foo2" << std::endl;
		else
			std::cout << "foo1 != foo2" << std::endl;
		*/
	}

	/// Performance test
	{
		std::ifstream ifs( "twitter.json" );
		std::string str( ( std::istreambuf_iterator<char>( ifs ) ), std::istreambuf_iterator<char>() );

		Stopwatch sw{ "Parse twitter.json 10x" };

		for ( int i = 0; i < 10; ++i )
		{
			json5::document doc;
			if ( auto err = json5::from_string( str, doc ) )
				break;
		}
	}

	return 0;
}
