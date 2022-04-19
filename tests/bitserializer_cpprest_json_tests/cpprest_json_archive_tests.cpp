﻿/*******************************************************************************
* Copyright (C) 2018-2021 by Pavel Kisliak                                     *
* This file is part of BitSerializer library, licensed under the MIT license.  *
*******************************************************************************/
#include "bitserializer/cpprestjson_archive.h"
#include "test_helpers/common_test_methods.h"
#include "test_helpers/common_json_test_methods.h"

using BitSerializer::Json::CppRest::JsonArchive;

//-----------------------------------------------------------------------------
// Tests of serialization for fundamental types (at root scope of archive)
//-----------------------------------------------------------------------------
TEST(JsonRestCpp, SerializeBoolean) {
	TestSerializeType<JsonArchive, bool>(false);
	TestSerializeType<JsonArchive, bool>(true);
}

TEST(JsonRestCpp, SerializeInteger) {
	TestSerializeType<JsonArchive, uint8_t>(std::numeric_limits<uint8_t>::min());
	TestSerializeType<JsonArchive, uint8_t>(std::numeric_limits<uint8_t>::max());
	TestSerializeType<JsonArchive, int64_t>(std::numeric_limits<int64_t>::min());
	TestSerializeType<JsonArchive, uint64_t>(std::numeric_limits<uint64_t>::max());
}

TEST(JsonRestCpp, SerializeFloat) {
	TestSerializeType<JsonArchive, float>(std::numeric_limits<float>::min());
	TestSerializeType<JsonArchive, float>(std::numeric_limits<float>::max());
}

TEST(JsonRestCpp, SerializeDouble) {
	TestSerializeType<JsonArchive, double>(std::numeric_limits<double>::min());
	TestSerializeType<JsonArchive, double>(std::numeric_limits<double>::max());
}

TEST(JsonRestCpp, SerializeNullptr)
{
	TestSerializeType<JsonArchive, std::nullptr_t>(nullptr);
}

//-----------------------------------------------------------------------------
// Tests of serialization any of std::string (at root scope of archive)
//-----------------------------------------------------------------------------
TEST(JsonRestCpp, SerializeUtf8Sting) {
	TestSerializeType<JsonArchive, std::string>("Test ANSI string");
	TestSerializeType<JsonArchive, std::string>(u8"Test UTF8 string - Привет мир!");
}

TEST(JsonRestCpp, SerializeUnicodeString) {
	TestSerializeType<JsonArchive, std::wstring>(L"Test wide string - Привет мир!");
	TestSerializeType<JsonArchive, std::u16string>(u"Test UTF-16 string - Привет мир!");
	TestSerializeType<JsonArchive, std::u32string>(U"Test UTF-32 string - Привет мир!");
}

TEST(JsonRestCpp, SerializeEnum) {
	TestSerializeType<JsonArchive, TestEnum>(TestEnum::Two);
}

//-----------------------------------------------------------------------------
// Tests of serialization for c-arrays (at root scope of archive)
//-----------------------------------------------------------------------------
TEST(JsonRestCpp, SerializeArrayOfBooleans) {
	TestSerializeArray<JsonArchive, bool>();
}

TEST(JsonRestCpp, SerializeArrayOfIntegers) {
	TestSerializeArray<JsonArchive, int8_t>();
	TestSerializeArray<JsonArchive, int64_t>();
}

TEST(JsonRestCpp, SerializeArrayOfFloats) {
	TestSerializeArray<JsonArchive, float>();
	TestSerializeArray<JsonArchive, double>();
}

TEST(JsonRestCpp, SerializeArrayOfNullptrs)
{
	TestSerializeArray<JsonArchive, std::nullptr_t>();
}

TEST(JsonRestCpp, SerializeArrayOfStrings) {
	TestSerializeArray<JsonArchive, std::string>();
}

TEST(JsonRestCpp, SerializeArrayOfUnicodeStrings) {
	TestSerializeArray<JsonArchive, std::wstring>();
	TestSerializeArray<JsonArchive, std::u16string>();
	TestSerializeArray<JsonArchive, std::u32string>();
}

TEST(JsonRestCpp, SerializeArrayOfClasses) {
	TestSerializeArray<JsonArchive, TestPointClass>();
}

TEST(JsonRestCpp, SerializeTwoDimensionalArray) {
	TestSerializeTwoDimensionalArray<JsonArchive, int32_t>();
}

//-----------------------------------------------------------------------------
// Tests of serialization for classes
//-----------------------------------------------------------------------------
TEST(JsonRestCpp, SerializeClassWithMemberBoolean) {
	TestSerializeClass<JsonArchive>(TestClassWithSubTypes<bool>(false));
	TestSerializeClass<JsonArchive>(TestClassWithSubTypes<bool>(true));
}

TEST(JsonRestCpp, SerializeClassWithMemberInteger) {
	TestSerializeClass<JsonArchive>(BuildFixture<TestClassWithSubTypes<int8_t, uint8_t, int64_t, uint64_t>>());
	TestSerializeClass<JsonArchive>(TestClassWithSubTypes(std::numeric_limits<int64_t>::min(), std::numeric_limits<uint64_t>::max()));
}

TEST(JsonRestCpp, SerializeClassWithMemberFloat) {
	TestSerializeClass<JsonArchive>(TestClassWithSubTypes(std::numeric_limits<float>::min(), 0.0f, std::numeric_limits<float>::max()));
}

TEST(JsonRestCpp, SerializeClassWithMemberDouble) {
	TestSerializeClass<JsonArchive>(TestClassWithSubTypes(std::numeric_limits<double>::min(), 0.0, std::numeric_limits<double>::max()));
}

TEST(JsonRestCpp, SerializeClassWithMemberNullptr)
{
	TestSerializeClass<JsonArchive>(BuildFixture<TestClassWithSubTypes<std::nullptr_t>>());
}

TEST(JsonRestCpp, SerializeClassWithMemberString) {
	TestSerializeClass<JsonArchive>(BuildFixture<TestClassWithSubTypes<std::string, std::wstring, std::u16string, std::u32string>>());
}

TEST(JsonRestCpp, SerializeClassHierarchy) {
	TestSerializeClass<JsonArchive>(BuildFixture<TestClassWithInheritance>());
}

TEST(JsonRestCpp, SerializeClassWithMemberClass) {
	using TestClassType = TestClassWithSubTypes<TestClassWithSubTypes<int64_t>>;
	TestSerializeClass<JsonArchive>(BuildFixture<TestClassType>());
}

TEST(JsonRestCpp, SerializeClassWithSubArray) {
	TestSerializeClass<JsonArchive>(BuildFixture<TestClassWithSubArray<int64_t>>());
}

TEST(JsonRestCpp, SerializeClassWithSubArrayOfClasses) {
	TestSerializeClass<JsonArchive>(BuildFixture<TestClassWithSubArray<TestPointClass>>());
}

TEST(JsonRestCpp, SerializeClassWithSubTwoDimArray) {
	TestSerializeClass<JsonArchive>(BuildFixture<TestClassWithSubTwoDimArray<int32_t>>());
}

TEST(JsonRestCpp, ShouldIterateKeysInObjectScope) {
	TestIterateKeysInObjectScope<JsonArchive>();
}

//-----------------------------------------------------------------------------
// Test paths in archive
//-----------------------------------------------------------------------------
TEST(JsonRestCpp, ShouldReturnPathInObjectScopeWhenLoading) {
	TestGetPathInJsonObjectScopeWhenLoading<JsonArchive>();
}

TEST(JsonRestCpp, ShouldReturnPathInObjectScopeWhenSaving) {
	TestGetPathInJsonObjectScopeWhenSaving<JsonArchive>();
}

TEST(JsonRestCpp, ShouldReturnPathInArrayScopeWhenLoading) {
	TestGetPathInJsonArrayScopeWhenLoading<JsonArchive>();
}

TEST(JsonRestCpp, ShouldReturnPathInArrayScopeWhenSaving) {
	TestGetPathInJsonArrayScopeWhenSaving<JsonArchive>();
}

//-----------------------------------------------------------------------------
// Test the validation for named values (boolean result, which returns by archive's method SerializeValue()).
//-----------------------------------------------------------------------------
TEST(JsonRestCpp, ShouldCollectErrorsAboutRequiredNamedValues) {
	TestValidationForNamedValues<JsonArchive, TestClassForCheckValidation<bool>>();
	TestValidationForNamedValues<JsonArchive, TestClassForCheckValidation<int>>();
	TestValidationForNamedValues<JsonArchive, TestClassForCheckValidation<double>>();
	TestValidationForNamedValues<JsonArchive, TestClassForCheckValidation<std::string>>();
	TestValidationForNamedValues<JsonArchive, TestClassForCheckValidation<TestPointClass>>();
	TestValidationForNamedValues<JsonArchive, TestClassForCheckValidation<int[3]>>();
}

TEST(JsonRestCpp, ShouldCollectErrorsWhenLoadingFromNotCompatibleTypes)
{
	using SourceStringType = TestClassForCheckCompatibleTypes<std::string>;
	TestValidationForNotCompatibleTypes<JsonArchive, SourceStringType, TestClassForCheckCompatibleTypes<std::nullptr_t>>();
	TestValidationForNotCompatibleTypes<JsonArchive, SourceStringType, TestClassForCheckCompatibleTypes<bool>>();
	TestValidationForNotCompatibleTypes<JsonArchive, SourceStringType, TestClassForCheckCompatibleTypes<int>>();
	TestValidationForNotCompatibleTypes<JsonArchive, SourceStringType, TestClassForCheckCompatibleTypes<double>>();
	TestValidationForNotCompatibleTypes<JsonArchive, SourceStringType, TestClassForCheckCompatibleTypes<TestPointClass>>();
	TestValidationForNotCompatibleTypes<JsonArchive, SourceStringType, TestClassForCheckCompatibleTypes<int[3]>>();
}

//-----------------------------------------------------------------------------
// Tests streams / files
//-----------------------------------------------------------------------------
TEST(JsonRestCpp, SerializeClassToStream) {
	TestSerializeClassToStream<JsonArchive, char>(BuildFixture<TestPointClass>());
}

TEST(JsonRestCpp, SerializeUnicodeToEncodedStream) {
	TestClassWithSubType<std::wstring> TestValue(L"Привет мир!");
	TestSerializeClassToStream<JsonArchive, char>(TestValue);
}

TEST(JsonRestCpp, LoadFromUtf8Stream) {
	TestLoadJsonFromEncodedStream<JsonArchive, BitSerializer::Convert::Utf8>(false);
}

TEST(JsonRestCpp, LoadFromUtf8StreamWithBom) {
	TestLoadJsonFromEncodedStream<JsonArchive, BitSerializer::Convert::Utf8>(true);
}

TEST(JsonRestCpp, SaveToUtf8Stream) {
	TestSaveJsonToEncodedStream<JsonArchive, BitSerializer::Convert::Utf8>(false);
}

TEST(JsonRestCpp, SaveToUtf8StreamWithBom) {
	TestSaveJsonToEncodedStream<JsonArchive, BitSerializer::Convert::Utf8>(true);
}

TEST(JsonRestCpp, SerializeClassToFile) {
	TestSerializeClassToFile<JsonArchive>(BuildFixture<TestPointClass>());
}

//-----------------------------------------------------------------------------
// Tests of errors handling
//-----------------------------------------------------------------------------
TEST(JsonRestCpp, ThrowExceptionWhenBadSyntaxInSource) {
	int testInt = 0;
	EXPECT_THROW(BitSerializer::LoadObject<JsonArchive>(testInt, "10 }}"), BitSerializer::SerializationException);
}