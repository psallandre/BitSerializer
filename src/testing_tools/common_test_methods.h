/*******************************************************************************
* Copyright (C) 2018-2024 by Pavel Kisliak                                     *
* This file is part of BitSerializer library, licensed under the MIT license.  *
*******************************************************************************/
#pragma once
#include <filesystem>
#include <optional>
#include <map>
#include "common_test_entities.h"
#include "bitserializer/types/std/vector.h"
#include "bitserializer/types/std/array.h"


/// <summary>
/// Approximately compares two floating point numbers based on passed epsilon.
/// </summary>
template <typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
static bool ApproximatelyEqual(T a, T b, T epsilon = std::numeric_limits<T>::epsilon())
{
	return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

/// <summary>
/// Test template of serialization to root scope of archive (single value types).
/// </summary>
/// <param name="value">The value.</param>
template <typename TArchive, typename T>
void TestSerializeType(T&& value)
{
	// Arrange
	typename TArchive::preferred_output_format outputArchive;
	std::remove_reference_t<T> actual;

	// Act
	BitSerializer::SaveObject<TArchive>(value, outputArchive);
	BitSerializer::LoadObject<TArchive>(actual, outputArchive);

	// Assert
	GTestExpectEq(std::forward<T>(value), actual);
}

/// <summary>
/// Test template of serialization single values with loading to different type.
/// </summary>
/// <param name="value">Source test value.</param>
/// <param name="expected">Expected value</param>
template <typename TArchive, typename TSource, typename TExpected>
void TestLoadingToDifferentType(TSource&& value, const TExpected& expected)
{
	// Arrange
	typename TArchive::preferred_output_format outputArchive;
	TExpected actual{};

	// Act
	BitSerializer::SaveObject<TArchive>(value, outputArchive);
	BitSerializer::LoadObject<TArchive>(actual, outputArchive);

	// Assert
	GTestExpectEq(expected, actual);
}

/// <summary>
/// Test template of serialization for c-array.
/// </summary>
template<typename TArchive, typename TValue, size_t SourceArraySize = 7, size_t TargetArraySize = 7>
void TestSerializeArray()
{
	// Arrange
	TValue testArray[SourceArraySize];
	BuildFixture(testArray);
	typename TArchive::preferred_output_format outputArchive;
	TValue actual[TargetArraySize];

	// Act
	BitSerializer::SaveObject<TArchive>(testArray, outputArchive);
	BitSerializer::LoadObject<TArchive>(actual, outputArchive);

	// Assert
	for (size_t i = 0; i < std::min(SourceArraySize, TargetArraySize); i++)
	{
		GTestExpectEq(testArray[i], actual[i]);
	}
}

/// <summary>
/// Test template of serialization for c-array.
/// </summary>
template<typename TArchive, typename TValue, size_t SourceArraySize = 7, size_t TargetArraySize = 7>
void TestSerializeArrayWithKey()
{
	// Arrange
	TValue testArray[SourceArraySize];
	BuildFixture(testArray);
	typename TArchive::preferred_output_format outputArchive;
	TValue actual[TargetArraySize];

	// Act
	BitSerializer::SaveObject<TArchive>(BitSerializer::KeyValue(L"Root", testArray), outputArchive);
	BitSerializer::LoadObject<TArchive>(BitSerializer::KeyValue(L"Root", actual), outputArchive);

	// Assert
	for (size_t i = 0; i < std::min(SourceArraySize, TargetArraySize); i++) {
		GTestExpectEq(testArray[i], actual[i]);
	}
}

/// <summary>
/// Test template of serialization for two-dimensional c-array.
/// </summary>
template<typename TArchive, typename TValue, size_t ArraySize1 = 3, size_t ArraySize2 = 5>
void TestSerializeTwoDimensionalArray()
{
	// Arrange
	TValue testArray[ArraySize1][ArraySize2];
	BuildFixture(testArray);
	typename TArchive::preferred_output_format outputArchive;
	TValue actual[ArraySize1][ArraySize2];

	// Act
	BitSerializer::SaveObject<TArchive>(testArray, outputArchive);
	BitSerializer::LoadObject<TArchive>(actual, outputArchive);

	// Assert
	for (size_t i = 0; i < ArraySize1; i++) {
		for (size_t c = 0; c < ArraySize2; c++) {
			GTestExpectEq(testArray[i][c], actual[i][c]);
		}
	}
}

/// <summary>
/// Test template of serialization for class with key (must have constant method Assert()).
/// </summary>
/// <param name="value">The value.</param>
template <typename TArchive, typename T>
void TestSerializeClassWithKey(T&& value)
{
	// Arrange
	typename TArchive::preferred_output_format outputArchive;
	std::decay_t<T> actual;

	// Act
	BitSerializer::SaveObject<TArchive>(BitSerializer::KeyValue(L"Root", value), outputArchive);
	BitSerializer::LoadObject<TArchive>(BitSerializer::KeyValue("Root", actual), outputArchive);

	// Assert
	value.Assert(actual);
}

/// <summary>
/// Test template of serialization for class with using streams.
/// </summary>
/// <param name="value">The value.</param>
template <typename TArchive, typename TStreamElem, typename T>
void TestSerializeClassToStream(T&& value)
{
	// Arrange
	using string_stream_type = std::basic_stringstream<TStreamElem, std::char_traits<TStreamElem>, std::allocator<TStreamElem>>;
	string_stream_type outputStream;
	std::decay_t<T> actual;

	// Act
	BitSerializer::SaveObject<TArchive>(value, outputStream);
	outputStream.seekg(0, std::ios::beg);
	BitSerializer::LoadObject<TArchive>(actual, outputStream);

	// Assert
	value.Assert(actual);
}

/// <summary>
/// Test template of serialization for array with using streams.
/// </summary>
template <typename TArchive, typename TStreamElem, typename T, size_t ArraySize = 3>
void TestSerializeArrayToStream(T(&testArray)[ArraySize])
{
	// Arrange
	using string_stream_type = std::basic_stringstream<TStreamElem, std::char_traits<TStreamElem>, std::allocator<TStreamElem>>;
	string_stream_type outputStream;
	T actual[ArraySize];

	// Act
	BitSerializer::SaveObject<TArchive>(testArray, outputStream);
	std::string str = outputStream.str();
	BitSerializer::LoadObject<TArchive>(actual, outputStream);

	// Assert
	for (size_t i = 0; i < ArraySize; i++)
	{
		testArray[i].Assert(actual[i]);
	}
}

/// <summary>
/// Test template of serialization to file.
/// </summary>
template <typename TArchive, size_t ArraySize = 3>
void TestSerializeArrayToFile()
{
	// Arrange
	auto path = std::filesystem::temp_directory_path() / "TestArchive.data";
	TestPointClass testArray[ArraySize], actual[ArraySize];
	BuildFixture(testArray);

	// Act
	BitSerializer::SaveObjectToFile<TArchive>(testArray, path);
	BitSerializer::LoadObjectFromFile<TArchive>(actual, path);

	// Assert
	for (size_t i = 0; i < ArraySize; i++)
	{
		testArray[i].Assert(actual[i]);
	}
}

/// <summary>
/// Test template of serialization for STL containers.
/// </summary>
template <typename TArchive, typename TContainer>
void TestSerializeStlContainer()
{
	// Arrange
	typename TArchive::preferred_output_format outputArchive;
	TContainer expected{};
	::BuildFixture(expected);
	TContainer actual{};

	// Act
	auto archiveData = BitSerializer::SaveObject<TArchive>(expected);
	BitSerializer::LoadObject<TArchive>(actual, archiveData);

	// Assert
	GTestExpectEq(expected, actual);
}

/// <summary>
/// Template for test loading to not empty container.
/// </summary>
template <typename TArchive, typename TContainer>
void TestLoadToNotEmptyContainer(size_t targetContainerSize)
{
	// Arrange
	typename TArchive::preferred_output_format outputArchive;
	TContainer expected{};
	::BuildFixture(expected);
	TContainer actual(targetContainerSize);

	// Act
	auto archiveData = BitSerializer::SaveObject<TArchive>(expected);
	BitSerializer::LoadObject<TArchive>(actual, archiveData);

	// Assert
	EXPECT_EQ(expected, actual);
}

/// <summary>
/// Test template of serialization for STL containers with custom assert function.
/// </summary>
/// <param name="assertFunc">The assertion function.</param>
template <typename TArchive, typename TContainer>
void TestSerializeStlContainer(std::function<void(const TContainer&, const TContainer&)> assertFunc)
{
	// Arrange
	typename TArchive::preferred_output_format outputArchive;
	TContainer expected{};
	::BuildFixture(expected);
	TContainer actual{};

	// Act
	auto archiveData = BitSerializer::SaveObject<TArchive>(expected);
	BitSerializer::LoadObject<TArchive>(actual, archiveData);

	// Assert
	assertFunc(expected, actual);
}

/// <summary>
/// Asserts the multimap container.
/// </summary>
/// <param name="expected">The expected.</param>
/// <param name="actual">The actual.</param>
template <typename TContainer>
void AssertMultimap(const TContainer& expected, const TContainer& actual)
{
	ASSERT_EQ(expected.size(), actual.size());
	// Order of values can be rearranged after loading
	for (auto& elem : actual) {
		auto expectedElementsRange = expected.equal_range(elem.first);
		auto result = std::find(expectedElementsRange.first, expectedElementsRange.second, elem);
		ASSERT_TRUE(result != expectedElementsRange.second);
	}
}

/// <summary>
/// Test template of validation for named values (boolean result, which returns by archive's method SerializeValue()).
/// </summary>
template <typename TArchive, class T>
void TestValidationForNamedValues()
{
	// Arrange
	T testObj[1];
	BuildFixture(testObj);
	typename TArchive::preferred_output_format outputArchive;

	// Act
	bool result = false;
	BitSerializer::SaveObject<TArchive>(testObj, outputArchive);
	try
	{
		BitSerializer::LoadObject<TArchive>(testObj, outputArchive);
		result = true;
	}
	catch (BitSerializer::ValidationException& ex)
	{
		// Assert
		EXPECT_EQ(BitSerializer::SerializationErrorCode::FailedValidation, ex.GetErrorCode());
		EXPECT_EQ(1, ex.GetValidationErrors().size());
	}

	EXPECT_FALSE(result);
}

/// <summary>
/// Template for test overflow target value when deserialization.
/// </summary>
template <typename TArchive, class TSourceType, class TTargetType>
void TestOverflowNumberPolicy(BitSerializer::OverflowNumberPolicy overflowNumberPolicy)
{
	// Arrange
	static_assert(std::is_arithmetic_v<TSourceType> && std::is_arithmetic_v<TTargetType>);
	static_assert(sizeof (TSourceType) >= sizeof (TTargetType));

	TSourceType testValue;
	if constexpr (std::is_floating_point_v<TTargetType>)
	{
		if constexpr (std::is_floating_point_v<TTargetType>) {
			testValue = TSourceType(std::numeric_limits<TTargetType>::max()) * 1.00001;
		}
		else {
			testValue = std::numeric_limits<TSourceType>::min();
		}
	}
	else
	{
		if constexpr (std::is_floating_point_v<TSourceType>)
		{
			// Test cast from floating point number to integer
			testValue = TSourceType(3.141592654f);
		}
		else
		{
			if constexpr (std::is_signed_v<TTargetType>) {
				testValue = TSourceType(std::numeric_limits<TTargetType>::min()) - 1;
			}
			else {
				testValue = TSourceType(std::numeric_limits<TTargetType>::max()) + 1;
			}
		}
	}
	TestClassWithSubTypes<TSourceType, TSourceType> sourceObj[1]{ { testValue, BuildFixture<TSourceType>() } };
	TestClassWithSubTypes<TTargetType, TSourceType> targetObj[1];
	targetObj->WithRequired();

	BitSerializer::SerializationOptions options;
	options.overflowNumberPolicy = overflowNumberPolicy;
	typename TArchive::preferred_output_format outputArchive;
	BitSerializer::SaveObject<TArchive>(sourceObj, outputArchive);

	// Act / Assert
	switch (overflowNumberPolicy)
	{
	case BitSerializer::OverflowNumberPolicy::ThrowError:
		try
		{
			BitSerializer::LoadObject<TArchive>(targetObj, outputArchive, options);
			EXPECT_TRUE(false);
		}
		catch (const BitSerializer::SerializationException& ex)
		{
			EXPECT_EQ(BitSerializer::SerializationErrorCode::Overflow, ex.GetErrorCode());
		}
		break;

	case BitSerializer::OverflowNumberPolicy::Skip:
		try
		{
			BitSerializer::LoadObject<TArchive>(targetObj, outputArchive, options);
			EXPECT_TRUE(false);
		}
		catch (const BitSerializer::ValidationException& ex)
		{
			EXPECT_EQ(BitSerializer::SerializationErrorCode::FailedValidation, ex.GetErrorCode());
			EXPECT_EQ(1, ex.GetValidationErrors().size());
		}
		// Second value should be loaded
		GTestExpectEq(std::get<1>(sourceObj[0]), std::get<1>(targetObj[0]));
		break;
	}
}

/// <summary>
/// Template for test loading mismatched types (e.g. number from string).
/// </summary>
template <typename TArchive, class TSourceType, class TTargetType>
void TestMismatchedTypesPolicy(BitSerializer::MismatchedTypesPolicy mismatchedTypesPolicy)
{
	// Arrange
	static_assert(!std::is_same_v<TSourceType, TTargetType>);

	// Use array with one object for be compatible with CSV
	TestClassWithSubTypes<TSourceType, TTargetType> sourceObj[1];
	BuildFixture(sourceObj);
	TestClassWithSubTypes<TTargetType, TTargetType> targetObj[1];
	// Loading with "Required" validator for force throw ValidationException
	targetObj->WithRequired();

	BitSerializer::SerializationOptions options;
	options.mismatchedTypesPolicy = mismatchedTypesPolicy;
	typename TArchive::preferred_output_format outputArchive;
	BitSerializer::SaveObject<TArchive>(sourceObj, outputArchive);

	// Act / Assert
	try
	{
		BitSerializer::LoadObject<TArchive>(targetObj, outputArchive, options);
		EXPECT_TRUE(false);
	}
	catch (const BitSerializer::ValidationException& ex)
	{
		// Loading from Null values should be excluded from MismatchedTypesPolicy processing
		if (mismatchedTypesPolicy == BitSerializer::MismatchedTypesPolicy::ThrowError && !std::is_same_v<TSourceType, std::nullptr_t>)
		{
			EXPECT_EQ(BitSerializer::SerializationErrorCode::MismatchedTypes, ex.GetErrorCode());
		}
		else
		{
			EXPECT_EQ(BitSerializer::SerializationErrorCode::FailedValidation, ex.GetErrorCode());
			EXPECT_EQ(1, ex.GetValidationErrors().size());
		}
		// Second value should be loaded
		GTestExpectEq(std::get<1>(sourceObj[0]), std::get<1>(targetObj[0]));
	}
	catch (const BitSerializer::SerializationException& ex)
	{
		// Loading from Null values should be excluded from MismatchedTypesPolicy processing
		if (mismatchedTypesPolicy == BitSerializer::MismatchedTypesPolicy::ThrowError && !std::is_same_v<TSourceType, std::nullptr_t>) {
			EXPECT_EQ(BitSerializer::SerializationErrorCode::MismatchedTypes, ex.GetErrorCode());
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
}


/// <summary>
/// Template for test visiting keys in the object scope.
/// </summary>
template <typename TArchive>
void TestVisitKeysInObjectScope(bool skipValues=false)
{
	// Arrange
	TestPointClass testObj[1];
	::BuildFixture(testObj);
	const std::map<typename TArchive::key_type, decltype(testObj->x), std::less<>> expectedValues {
		{ BitSerializer::Convert::To<typename TArchive::key_type>("x"), testObj->x },
		{ BitSerializer::Convert::To<typename TArchive::key_type>("y"), testObj->y }
	};

	using OutputFormat = typename TArchive::preferred_output_format;
	OutputFormat outputData;
	BitSerializer::SaveObject<TArchive>(testObj, outputData);
	const BitSerializer::SerializationOptions options;
	BitSerializer::SerializationContext context(options);
	typename TArchive::input_archive_type inputArchive(static_cast<const OutputFormat&>(outputData), context);

	// Act / Assert
	auto arrScope = inputArchive.OpenArrayScope(std::size(testObj));
	ASSERT_TRUE(arrScope.has_value());
	auto objScope = arrScope->OpenObjectScope(0);
	ASSERT_TRUE(objScope.has_value());

	size_t index = 0;
	objScope->VisitKeys([&objScope, &expectedValues, &index, &skipValues](auto&& key)
	{
		using T = std::decay_t<decltype(key)>;
		ASSERT_TRUE(index < expectedValues.size());
		if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
		{
			auto it = expectedValues.find(key);
			ASSERT_TRUE(it != expectedValues.cend());

			if (!skipValues)
			{
				decltype(testObj->x) actualValue;
				objScope->SerializeValue(key, actualValue);
				EXPECT_EQ(it->second, actualValue);
			}
		}
		++index;
	});
	EXPECT_EQ(expectedValues.size(), index);
}
