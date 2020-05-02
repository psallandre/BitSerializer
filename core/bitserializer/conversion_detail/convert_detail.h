/*******************************************************************************
* Copyright (C) 2020 by Pavel Kisliak                                          *
* This file is part of BitSerializer library, licensed under the MIT license.  *
*******************************************************************************/
#pragma once
#include <string>
#include <type_traits>
#include <filesystem>

#include "convert_fundamental.h"
#include "convert_utf.h"
#include "object_traits.h"
#include "convert_enum.h"

namespace BitSerializer::Convert::Detail {

//------------------------------------------------------------------------------
// Convert to types which have the same base char type
//------------------------------------------------------------------------------
inline void To(const char* in_str, std::string& ret_Str)				{ ret_Str = in_str; }
inline void To(const wchar_t* in_str, std::wstring& ret_Str)			{ ret_Str = in_str; }
inline void To(const std::string& in_str, std::string& ret_Str)			{ ret_Str = in_str; }
inline void To(const std::wstring& in_str, std::wstring& ret_Str)		{ ret_Str = in_str; }

//------------------------------------------------------------------------------
// Convert std::string to std::wstring and vice versa (with using UTF-8 encoding)
//------------------------------------------------------------------------------
inline void To(const std::wstring& in_str, std::string& ret_Str)
{
	ret_Str.reserve(ret_Str.size() + (in_str.size() * 2));
	Utf8::Encode(in_str.begin(), in_str.end(), ret_Str);
}

inline void To(const std::string& in_str, std::wstring& ret_Str)
{
	ret_Str.reserve(ret_Str.size() + in_str.size());
	Utf8::Decode(in_str.begin(), in_str.end(), ret_Str);
}

//------------------------------------------------------------------------------
// Convert enumeration types
// Need to register your enumeration types, see detail in class ConvertEnum.
//------------------------------------------------------------------------------
template <typename T, typename TSym, typename TAllocator, std::enable_if_t<std::is_enum_v<T>, int> = 0>
bool To(T val, std::basic_string<TSym, std::char_traits<TSym>, TAllocator>& ret_Str)
{
	return ConvertEnum::ToString<T>(val, ret_Str);
}
template <typename T, typename TSym, typename TAllocator, std::enable_if_t<std::is_enum_v<T>, int> = 0>
bool To(const std::basic_string<TSym, std::char_traits<TSym>, TAllocator>& str, T& ret_Val)
{
	return ConvertEnum::FromString<T>(str, ret_Val);
}

//------------------------------------------------------------------------------
// Convert fundamental types
//------------------------------------------------------------------------------
template <typename T, typename TSym, typename TAllocator, std::enable_if_t<std::is_fundamental_v<T>, int> = 0>
void To(T val, std::basic_string<TSym, std::char_traits<TSym>, TAllocator>& ret_Str)
{
	Fundamental::To(val, ret_Str);
}
template <typename T, typename TSym, typename TAllocator, std::enable_if_t<std::is_fundamental_v<T>, int> = 0>
void To(const std::basic_string<TSym, std::char_traits<TSym>, TAllocator>& str, T& ret_Val)
{
	Fundamental::To(str, ret_Val);
}

//-----------------------------------------------------------------------------
// Convert classes and unions (convert methods should be implemented in concrete classes)
//-----------------------------------------------------------------------------
template <class T, typename TAllocator, std::enable_if_t<(std::is_class_v<T> || std::is_union_v<T>), int> = 0>
void To(const T& classRef, std::basic_string<char, std::char_traits<char>, TAllocator>& ret_Str)
{
	constexpr auto isConvertible = has_to_string_v<T, std::basic_string<char, std::char_traits<char>, TAllocator>>;
	static_assert(isConvertible, "Class should has public constant methods ToString() or static in namespace BitSerializer::Convert::Detail.");

	if constexpr (isConvertible) {
		ret_Str = classRef.ToString();
	}
}

template <class T, typename TAllocator, std::enable_if_t<(std::is_class_v<T> || std::is_union_v<T>), int> = 0>
void To(const T& classRef, std::basic_string<wchar_t, std::char_traits<wchar_t>, TAllocator>& ret_Str)
{
	constexpr auto isConvertible = has_to_string_v<T, std::basic_string<wchar_t, std::char_traits<wchar_t>, TAllocator>>;
	static_assert(isConvertible, "Class should has public constant methods ToWString() or static in namespace BitSerializer::Convert::Detail.");

	if constexpr (isConvertible) {
		ret_Str = classRef.ToWString();
	}
}

template <class T, typename TSym, typename TAllocator, std::enable_if_t<(std::is_class_v<T> || std::is_union_v<T>), int> = 0>
void To(const std::basic_string<TSym, std::char_traits<TSym>, TAllocator>& str, T& ret_Val)
{
	constexpr auto isConvertible = has_from_string_v<T, std::basic_string<TSym, std::char_traits<TSym>, TAllocator>>;
	static_assert(isConvertible, "Class should has public method FromString() or static in namespace BitSerializer::Convert::Detail.");

	if constexpr (isConvertible) {
		ret_Val.FromString(str);
	}
}

//-----------------------------------------------------------------------------
// Convert from filesystem::path to string
//-----------------------------------------------------------------------------
inline void To(const std::filesystem::path& path, std::string& ret_Str) {
	ret_Str.assign(path.generic_string());
}

inline void To(const std::filesystem::path& path, std::wstring& ret_Str) {
	ret_Str.assign(path.generic_wstring());
}

}	// BitSerializer::Convert::Detail
