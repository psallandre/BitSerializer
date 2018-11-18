# BitSerializer
___
The library is designed for simple serialization of arbitrary C++ types to various output formats. The historical purpose was to simplify the serialization of data for the http server. The good tests coverage helps to keep stability of project.

This is second release of library and it's still in active development, currently it includes support for only one JSON format but with two kind of implementation - one of them is based on RapidJson and second on CppRestSDK. If you are see kind of issue, please describe it in «[Issues](https://bitbucket.org/Pavel_Kisliak/bitserializer/issues?status=new&status=open)» section.

#### What's new in version 0.8:
- [ ! ] The package for VCPKG was splitted into two: "bitserializer" (core without any dependencies) and "bitserializer-cpprestjson" (requires "cpprestsdk").
- [ + ] Added new implementation for JSON format based on library RapidJson (currently supported only UTF16).
- [ + ] Added CMake support (it needs just for samples and tests, as the library is headers only).
- [ + ] Added validation of deserialized values.
- [ + ] Added directory with samples.
- [ + ] Added function MakeAutoKeyVale() to make key/value which is able to automatically adapt key to target archive.
- [ \* ] Enhanced architecture for support different kind of formats (for example allow to implement ANSI/Unicode streams in one archive).
- [ \* ] Fixed compilation issues on latest Visual Studio 15.8.6 and GCC.
- [ \* ] Changed (unified) interface methods: LoadObjectFromStream() -> LoadObject(), SaveObjectToStream() -> SaveObject().

[Full log of changes](History.md)

#### Main features:
- Flexible architecture, which allows to support different kind of formats (currently only JSON).
- Produces a clear JSON, which is convenient to use with Javascript.
- Simple syntax (similar to serialization in Boost library).
- Validation of deserialized values.
- Checking at compile time the permissibility of saving types depending on the structure of the output format.
- Support for serialization ANSI and wide strings.
- Support for serialization of most STL containers.
- Support for serialization of enum types (registration of a names map is required).
- As a bonus, the subsystem for converting strings to / from arbitrary types.

#### Supported Formats:
| VCPKG Library name | Format | Based on |
| ------ | ------ | ------ |
| bitserializer-cpprestjson | JSON | [C++ REST SDK](https://github.com/Microsoft/cpprestsdk) |
| bitserializer-rapidjson | JSON | [RapidJson](https://github.com/Tencent/rapidjson) |

#### Requirements:
  - C++ 17
  - Dependencies which required by selected type of archive.

#### How to install:
The library is contains only header files, but you should install one or more third party libraries which are depend from selected type of archive (please follow instructions for these libraries). If you are a Windows user, the best way is to use [Vcpkg manager](https://github.com/Microsoft/vcpkg), the dependent libraries would installed automatically. For example, if you'd like to use JSON serialization based on RapidJson, please execute this script:
```shell
vcpkg install bitserializer-rapidjson bitserializer-rapidjson:x64-windows
```
Now you need just include main file of BitSerializer which implements serialization and file, which implements required format (JSON for example).
```cpp
#include "bitserializer/bit_serializer.h"
#include "bitserializer_rapidjson/rapidjson_archive.h"
```

___
## Examples of using

#### Hello world!
[See full sample](samples/hello_world/hello_world.cpp)
```cpp
#include <cassert>
#include <iostream>
#include "bitserializer/bit_serializer.h"
#include "bitserializer_cpprest_json/cpprest_json_archive.h"

using namespace BitSerializer::Json::CppRest;

int main()
{
	std::string expected = "Hello world!";
	auto json = BitSerializer::SaveObject<JsonArchive>(expected);
	std::string result;
	BitSerializer::LoadObject<JsonArchive>(result, json);

	assert(result == expected);
	std::cout << result << std::endl;

	return EXIT_SUCCESS;
}
```
There is no mistake as JSON format supported any type at root level (and libraries which are used as base also supports this).

#### Save std::map
Due to the fact that the map key is used as a key in JSON, it must be convertible to a string (by default supported all of fundamental types), if you want to use your own class as a key, you can add conversion methods to it. You also can implement specialized Serialize() method in extreme cases.
```cpp
std::map<std::string, int> testMap = 
	{ { "One", 1 },{ "Two", 2 },{ "Three", 3 },{ "Four", 4 },{ "Five", 5 } };
auto jsonResult = BitSerializer::SaveObject<JsonArchive>(testMap);
```
Returns result
```json
{
	"Five": 5,
	"Four": 4,
	"One": 1,
	"Three": 3,
	"Two": 2
}
```
#### Loading a vector of maps
Input JSON
```json
[{
	"One": 1,
	"Three": 3,
	"Two": 2
}, {
	"Five": 5,
	"Four": 4
}]
```
Code:
```cpp
std::vector<std::map<std::string, int>> testVectorOfMaps;
const std::wstring inputJson = L"[{\"One\":1,\"Three\":3,\"Two\":2},{\"Five\":5,\"Four\":4}]";
BitSerializer::LoadObject<JsonArchive>(testVectorOfMaps, inputJson);
```

#### Serializing class
There are two ways to serialize a class:

  * Your own class (sources can be modified), possible to create internal or external method Serialize(), but internal is more convenient.
  * Third party class (no access to sources), only external method in namespace BitSerializer.

Next example demonstrates how to implement internal serialization method:
```cpp
#include "bitserializer/bit_serializer.h"
#include "bitserializer_cpprest_json/cpprest_json_archive.h"

using namespace BitSerializer;
using namespace BitSerializer::Json::CppRest;

class TestSimpleClass
{
public:
	TestSimpleClass()
	{
		testBool = true;
		testString = L"Hello world!";
		for (size_t i = 0; i < 3; i++) {
			for (size_t k = 0; k < 2; k++) {
				TestTwoDimensionArray[i][k] = i * 10 + k;
			}
		}
	}

	template <class TArchive>
	void Serialize(TArchive& archive)
	{
		archive << MakeKeyValue(L"testBool", testBool);
		archive << MakeKeyValue(L"testString", testString);
		archive << MakeKeyValue(L"TestTwoDimensionArray", TestTwoDimensionArray);
	};

private:
	bool testBool;
	std::wstring testString;
	size_t TestTwoDimensionArray[3][2];
};

int main()
{
	auto simpleObj = TestSimpleClass();
	auto result = BitSerializer::SaveObject<JsonArchive>(simpleObj);
    return 0;
}
```
Returns result
```json
{
	"testBool": true,
	"testString": "Hello world!",
	"TestTwoDimensionArray": [
		[0, 1],
		[10, 11],
		[20, 21]
	]
}
```

#### Serializing base class
To serialize the base class, use the helper method BaseObject(), as in the next example.
```cpp
template <class TArchive>
void Serialize(TArchive& archive)
{
	archive << BaseObject<MyBaseClass>(*this);
	archive << MakeKeyValue(L"TestInt", TestInt);
};
```

#### Serializing third party class
For serialize third party class, which source cannot be modified, need to implement two types of Serialize() methods in the namespace BitSerializer. The first method responsible to serialize a value with key, the second - without. This is a basic concept of BitSerializer which helps to control at compile time the possibility the type serialization in a current level of archive. For example, you can serialize any type to a root level of JSON, but you can't do it with key. In other case, when you in the object scope of JSON, you can serialize values only with keys.

[See full sample](samples/serialize_third_party_class/serialize_third_party_class.cpp)
```cpp
#include <iostream>
#include "bitserializer/bit_serializer.h"
#include "bitserializer_rapidjson/rapidjson_archive.h"

class TestThirdPartyClass
{
public:
	TestThirdPartyClass(int x, int y)
		: x(x), y(y)
	{ }

	int x, y;
};

namespace BitSerializer
{
	namespace Detail
	{
		class TestThirdPartyClassSerializer
		{
		public:
			TestThirdPartyClassSerializer(TestThirdPartyClass& value)
				: value(value)
			{ }

			template <class TArchive>
			inline void Serialize(TArchive& archive)
			{
				archive << MakeAutoKeyValue(L"x", value.x);
				archive << MakeAutoKeyValue(L"y", value.y);
			}

			TestThirdPartyClass& value;
		};
	}	// namespace Detail

	template<typename TArchive, typename TKey>
	inline void Serialize(TArchive& archive, TKey&& key, TestThirdPartyClass& value)
	{
		auto serializer = Detail::TestThirdPartyClassSerializer(value);
		Serialize(archive, key, serializer);
	}
	template<typename TArchive>
	inline void Serialize(TArchive& archive, TestThirdPartyClass& value)
	{
		auto serializer = Detail::TestThirdPartyClassSerializer(value);
		Serialize(archive, serializer);
	}
}	// namespace BitSerializer


using namespace BitSerializer::Json::RapidJson;

int main()
{
	auto testObj = TestThirdPartyClass(100, 200);
	auto result = BitSerializer::SaveObject<JsonArchive>(testObj);
	std::wcout << result << std::endl;
	return 0;
}
```

#### Serializing enum types
To be able to serialize enum types, you must register a map with string equivalents in the HEADER file.
```cpp
// file HttpMethods.h
#pragma once
#include "bitserializer\string_conversion.h"

enum class HttpMethod {
	Delete = 1,
	Get = 2,
	Head = 3
};

REGISTER_ENUM_MAP(HttpMethod)
{
	{ HttpMethod::Delete,   "delete" },
	{ HttpMethod::Get,      "get" },
	{ HttpMethod::Head,     "head" }
} END_ENUM_MAP()
```

#### Conditions for checking the serialization mode
To check the current serialization mode, use two methods - IsLoading() and IsSaving(). They are haven't CPU overhead, because they are «constexpr».
```cpp
class Foo
public:
    template <class TArchive>
    inline void Serialize(TArchive& archive)
    {
    	if constexpr (archive.IsLoading()) {
	        // Code which executes in loading mode
	    }
	    else {
    		// Code which executes in saving mode
    	}
	
    	if constexpr (archive.IsSaving()) {
		    // Code which executes in saving mode
	    }
	    else {
    		// Code which executes in loading mode
    	}
    }
}
```

#### Validation of deserialized values

BitSerializer allows to add an arbitrary number of validation rules to the named values, the syntax is quite simple:
```cpp
archive << MakeKeyValue("testFloat", testFloat, Required(), Range(-1.0f, 1.0f));
```
After deserialize, you can check the status in context and get errors:
```cpp
if (!Context.IsValid())
{
    const auto& validationErrors = Context.GetValidationErrors();
}
```
Basically implemented few validators: Required, Range, MinSize, MaxSize.
Validator 'Range' can be used with all types which have operators '<' and '>'.
Validators 'MinSize' and 'MaxSize' can be applied to all values which have size() method.
This list will be extended in future.

[See full sample](samples/validation/validation.cpp)
```cpp
#include <iostream>
#include "bitserializer/bit_serializer.h"
#include "bitserializer_rapidjson/rapidjson_archive.h"

using namespace BitSerializer;
using namespace BitSerializer::Json::RapidJson;

class TestSimpleClass
{
public:
	template <class TArchive>
	void Serialize(TArchive& archive)
	{
		archive << MakeKeyValue(L"TestBool", mTestBool, Required());
		archive << MakeKeyValue(L"TestInt", mTestInt, Required(), Range(0, 100));
		archive << MakeKeyValue(L"TestDouble", mTestDouble, Required(), Range(-1.0, 1.0));
		archive << MakeKeyValue(L"TestString", mTestString, MaxSize(8));
	};

private:
	bool mTestBool;
	int mTestInt;
	double mTestDouble;
	std::string mTestString;
};

int main()
{
	auto simpleObj = TestSimpleClass();
	auto json = L"{ \"TestInt\": 2000, \"TestDouble\": 1.0, \"TestString\" : \"Very looooooooong string!\" }";
	BitSerializer::LoadObject<JsonArchive>(simpleObj, json);
	if (!BitSerializer::Context.IsValid())
	{
		std::wcout << L"Validation errors: " << std::endl;
		const auto& validationErrors = BitSerializer::Context.GetValidationErrors();
		for (const auto& keyErrors : validationErrors)
		{
			std::wcout << L"Path: " << keyErrors.first << std::endl;
			for (const auto& err : keyErrors.second)
			{
				std::wcout << L"\t" << err << std::endl;
			}
		}
	}

	return EXIT_SUCCESS;
}
```

The result of execution this code:
```text
Validation errors:
Path: /TestBool
        This field is required
Path: /TestInt
        Value must be between 0 and 100
Path: /TestString
        The maximum size of this field should be not greater than 8
```
Returned paths for invalid values is dependent to archive type, in this sample it's JSON Pointer (RFC 6901).

#### Compile time checking
The new C++ 17 ability «if constexpr» helps to generate clear error messages.
If you try to serialize an object that is not supported at the current level of the archive, you will receive a simple error message.
```cpp
template <class TArchive>
inline void Serialize(TArchive& archive)
{
    // Error    C2338	BitSerializer. The archive doesn't support serialize fundamental type without key on this level.
    archive << testBool;
    // Proper use
	archive << MakeKeyValue(L"testString", testString);
};
```

#### Error handling
```cpp
try
{
	int testInt;
	BitSerializer::LoadObject<JsonArchive>(testInt, L"10 ?");
}
catch (const BitSerializer::SerializationException& ex)
{
	// Parsing error: Malformed token
	std::string message = ex.what();
}
```

License
----
MIT, Copyright (C) 2018 by Pavel Kisliak

The library currently was tested only on VS 2017 and still in development, please use it at your own risk.
