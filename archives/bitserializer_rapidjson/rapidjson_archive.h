/*******************************************************************************
* Copyright (C) 2018 by Pavel Kisliak                                          *
* This file is part of BitSerializer library, licensed under the MIT license.  *
*******************************************************************************/
#pragma once
#include <cassert>
#include <type_traits>
#include <optional>
#include <variant>
#include "bitserializer/serialization_detail/errors_handling.h"
#include "bitserializer/serialization_detail/media_archive_base.h"

// External dependency (RapidJson)
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

namespace BitSerializer::Json::RapidJson {
namespace Detail {

/// <summary>
/// The traits of JSON archive based on RapidJson
/// </summary>
class RapidJsonArchiveTraits
{
public:
	using key_type = std::wstring;
	using supported_key_types = SupportedKeyTypes<const wchar_t*, std::wstring>;
	using preferred_output_format = std::wstring;
	using preferred_stream_char_type = std::wostream::char_type;
	static const wchar_t path_separator = L'/';
};

// Forward declarations
template <SerializeMode TMode, class TAllocator>
class RapidJsonObjectScope;

/// <summary>
/// Base class of JSON scope
/// </summary>
/// <seealso cref="MediaArchiveBase" />
class RapidJsonScopeBase : public RapidJsonArchiveTraits
{
protected:
	using RapidJsonNode = rapidjson::GenericValue<rapidjson::UTF16<>>;

public:
	using key_type_view = std::basic_string_view<key_type::value_type>;

	explicit RapidJsonScopeBase(const RapidJsonNode* node, RapidJsonScopeBase* parent = nullptr, key_type_view parentKey = {})
		: mNode(const_cast<RapidJsonNode*>(node))
		, mParent(parent)
		, mParentKey(parentKey)
	{ }

	virtual ~RapidJsonScopeBase() = default;

	/// <summary>
	/// Gets the current path in JSON (RFC 6901 - JSON Pointer).
	/// </summary>
	/// <returns></returns>
	virtual std::wstring GetPath() const
	{
		const std::wstring localPath = mParentKey.empty()
			? std::wstring()
			: path_separator + Convert::ToWString(mParentKey);
		return mParent == nullptr ? localPath : mParent->GetPath() + localPath;
	}

protected:
	template <typename T, std::enable_if_t<std::is_fundamental_v<T>, int> = 0>
	static bool LoadFundamentalValue(const RapidJsonNode& jsonValue, T& value)
	{
		if (!jsonValue.IsNumber())
			return false;

		if constexpr (std::is_integral_v<T>)
		{
			if constexpr (std::is_same_v<T, int64_t>) {
				value = jsonValue.GetInt64();
			}
			else if constexpr (std::is_same_v<T, uint64_t>) {
				value = jsonValue.GetUint64();
			}
			else {
				value = static_cast<T>(jsonValue.GetInt());
			}
		}
		else
		{
			value = static_cast<T>(jsonValue.GetDouble());
		}
		return true;
	}

	template <typename T, std::enable_if_t<std::is_fundamental_v<T>, int> = 0>
	static bool SaveFundamentalValue(RapidJsonNode& jsonValue, T& value)
	{
		if constexpr (std::is_integral_v<T>)
		{
			if constexpr (std::is_same_v<T, int64_t>) {
				jsonValue.SetInt64(value);
			}
			else if constexpr (std::is_same_v<T, uint64_t>) {
				jsonValue.SetUint64(value);
			}
			else {
				jsonValue.SetInt(value);
			}
		}
		else
		{
			jsonValue.SetDouble(value);
		}
		return true;
	}

	template <typename TSym, typename TAllocator>
	static bool LoadString(const RapidJsonNode& jsonValue, std::basic_string<TSym, std::char_traits<TSym>, TAllocator>& value)
	{
		if (!jsonValue.IsString())
			return false;

		if constexpr (std::is_same_v<TSym, RapidJsonNode::EncodingType::Ch>)
			value = jsonValue.GetString();
		else
			value = Convert::To<std::basic_string<TSym, std::char_traits<TSym>, TAllocator>>(jsonValue.GetString());
		return true;
	}

	template <typename TSym, typename TAllocator, typename TRapidAllocator>
	static RapidJsonNode MakeRapidJsonNodeFromString(const std::basic_string<TSym, std::char_traits<TSym>, TAllocator>& value, TRapidAllocator& allocator)
	{
		using TargetSymType = RapidJsonNode::EncodingType::Ch;
		if constexpr (std::is_same_v<TSym, RapidJsonNode::EncodingType::Ch>)
			return RapidJsonNode(value.data(), static_cast<rapidjson::SizeType>(value.size()), allocator);
		else {
			const auto str = Convert::To<std::basic_string<TargetSymType, std::char_traits<TargetSymType>>>(value);
			return RapidJsonNode(str.data(), static_cast<rapidjson::SizeType>(str.size()), allocator);
		}
	}

	RapidJsonNode* mNode;
	RapidJsonScopeBase* mParent;
	key_type_view mParentKey;
};


/// <summary>
/// JSON scope for serializing arrays (list of values without keys).
/// </summary>
/// <seealso cref="RapidJsonScopeBase" />
template <SerializeMode TMode, class TAllocator>
class RapidJsonArrayScope : public ArchiveScope<TMode>, public RapidJsonScopeBase
{
public:
	using AllocatorType = TAllocator;

	explicit RapidJsonArrayScope(const RapidJsonNode* node, TAllocator& allocator, RapidJsonScopeBase* parent = nullptr, key_type_view parentKey = {})
		: RapidJsonScopeBase(node, parent, parentKey)
		, mAllocator(allocator)
		, mValueIt(mNode->GetArray().Begin())
	{
		assert(mNode->IsArray());
	}

	/// <summary>
	/// Returns the size of stored elements (for arrays and objects).
	/// </summary>
	size_t GetSize() const {
		return mNode->Capacity();
	}

	/// <summary>
	/// Gets the current path in JSON (RFC 6901 - JSON Pointer).
	/// </summary>
	std::wstring GetPath() const override
	{
		int64_t index;
		if constexpr (TMode == SerializeMode::Load)
			index = std::distance(mNode->Begin(), mValueIt);
		else
			index = mNode->GetArray().Size();
		return RapidJsonScopeBase::GetPath() + path_separator + Convert::ToWString(index == 0 ? 0 : index - 1);
	}

	void SerializeValue(bool& value)
	{
		if constexpr (TMode == SerializeMode::Load)
		{
			auto* jsonValue = NextElement();
			if (jsonValue != nullptr && jsonValue->IsBool())
				value = jsonValue->GetBool();
		}
		else {
			SaveJsonValue(RapidJsonNode(value));
		}
	}

	template <typename T, std::enable_if_t<std::is_fundamental_v<T>, int> = 0>
	void SerializeValue(T& value)
	{
		if constexpr (TMode == SerializeMode::Load)
		{
			auto* jsonValue = NextElement();
			if (jsonValue != nullptr)
				LoadFundamentalValue(*jsonValue, value);
		}
		else {
			SaveJsonValue(RapidJsonNode(value));
		}
	}

	template <typename TSym, typename TStrAllocator>
	void SerializeString(std::basic_string<TSym, std::char_traits<TSym>, TStrAllocator>& value)
	{
		if constexpr (TMode == SerializeMode::Load) {
			auto* jsonValue = NextElement();
			if (jsonValue != nullptr)
				LoadString(*jsonValue, value);
		}
		else {
			SaveJsonValue(RapidJsonScopeBase::MakeRapidJsonNodeFromString(value, mAllocator));
		}
	}

	std::optional<RapidJsonObjectScope<TMode, AllocatorType>> OpenObjectScope()
	{
		if constexpr (TMode == SerializeMode::Load)
		{
			auto* jsonValue = NextElement();
			if (jsonValue != nullptr && jsonValue->IsObject())
				return std::make_optional<RapidJsonObjectScope<TMode, AllocatorType>>(jsonValue, mAllocator, this);
			return std::nullopt;
		}
		else
		{
			SaveJsonValue(RapidJsonNode(rapidjson::kObjectType));
			auto& lastJsonValue = (*mNode)[mNode->Size() - 1];
			return std::make_optional<RapidJsonObjectScope<TMode, AllocatorType>>(&lastJsonValue, mAllocator, this);
		}
	}

	std::optional<Detail::RapidJsonArrayScope<TMode, AllocatorType>> OpenArrayScope(size_t arraySize)
	{
		if constexpr (TMode == SerializeMode::Load)
		{
			auto* jsonValue = NextElement();
			if (jsonValue != nullptr && jsonValue->IsArray())
				return std::make_optional<RapidJsonArrayScope<TMode, AllocatorType>>(jsonValue, mAllocator, this);
			return std::nullopt;
		}
		else
		{
			auto rapidJsonArray = RapidJsonNode(rapidjson::kArrayType);
			rapidJsonArray.Reserve(static_cast<rapidjson::SizeType>(arraySize), mAllocator);
			SaveJsonValue(std::move(rapidJsonArray));
			auto& lastJsonValue = (*mNode)[mNode->Size() - 1];
			return std::make_optional<RapidJsonArrayScope<TMode, AllocatorType>>(&lastJsonValue, mAllocator, this);
		}
	}

protected:
	inline const RapidJsonNode* NextElement()
	{
		if (mValueIt == mNode->End())
			return nullptr;
		const auto& jsonValue = *mValueIt;
		++mValueIt;
		return &jsonValue;
	}

	inline void SaveJsonValue(RapidJsonNode&& jsonValue) const
	{
		assert(mNode->Size() < mNode->Capacity());
		mNode->PushBack(std::move(jsonValue), mAllocator);
	}

	TAllocator& mAllocator;
	RapidJsonNode::ValueIterator mValueIt;
};

/// <summary>
/// Constant iterator of the keys.
/// </summary>
class key_const_iterator
{
	template <SerializeMode TMode, class TAllocator>
	friend class RapidJsonObjectScope;

	rapidjson::GenericValue<rapidjson::UTF16<>>::MemberIterator mJsonIt;

	key_const_iterator(rapidjson::GenericValue<rapidjson::UTF16<>>::MemberIterator it)
		: mJsonIt(it) { }

public:
	bool operator==(const key_const_iterator& rhs) const {
		return this->mJsonIt == rhs.mJsonIt;
	}
	bool operator!=(const key_const_iterator& rhs) const {
		return this->mJsonIt != rhs.mJsonIt;
	}

	key_const_iterator& operator++() {
		++mJsonIt;
		return *this;
	}

	const wchar_t* operator*() const {
		return mJsonIt->name.GetString();
	}
};

/// <summary>
/// JSON scope for serializing objects (list of values with keys).
/// </summary>
/// <seealso cref="RapidJsonScopeBase" />
template <SerializeMode TMode, class TAllocator>
class RapidJsonObjectScope : public ArchiveScope<TMode>, public RapidJsonScopeBase
{
public:
	using AllocatorType = TAllocator;

	explicit RapidJsonObjectScope(const RapidJsonNode* node, TAllocator& allocator, RapidJsonScopeBase* parent = nullptr, key_type_view parentKey = {})
		: RapidJsonScopeBase(node, parent, parentKey)
		, mAllocator(allocator)
	{
		assert(mNode->IsObject());
	};

	key_const_iterator cbegin() const {
		return key_const_iterator(mNode->GetObject().begin());
	}

	key_const_iterator cend() const {
		return key_const_iterator(mNode->GetObject().end());
	}

	inline bool SerializeValue(const key_type& key, bool& value) {
		return SerializeBooleanImpl(key, value);
	}
	inline bool SerializeValue(const wchar_t* key, bool& value) {
		return SerializeBooleanImpl(key, value);
	}

	template <typename T, std::enable_if_t<std::is_fundamental_v<T>, int> = 0>
	inline bool SerializeValue(const key_type& key, T& value) {
		return SerializeFundamentalValueImpl(key, value);
	}
	template <typename T, std::enable_if_t<std::is_fundamental_v<T>, int> = 0>
	inline bool SerializeValue(const wchar_t* key, T& value) {
		return SerializeFundamentalValueImpl(key, value);
	}

	template <typename TSym, typename TStrAllocator>
	inline bool SerializeString(const key_type& key, std::basic_string<TSym, std::char_traits<TSym>, TStrAllocator>& value) {
		return SerializeStringImpl(key, value);
	}
	template <typename TSym, typename TStrAllocator>
	inline bool SerializeString(const wchar_t* key, std::basic_string<TSym, std::char_traits<TSym>, TStrAllocator>& value) {
		return SerializeStringImpl(key, value);
	}

	inline std::optional<RapidJsonObjectScope<TMode, TAllocator>> OpenObjectScope(const key_type& key) {
		return OpenObjectScopeImpl(key);
	}
	inline std::optional<RapidJsonObjectScope<TMode, TAllocator>> OpenObjectScope(const wchar_t* key) {
		return OpenObjectScopeImpl(key);
	}

	inline std::optional<RapidJsonArrayScope<TMode, TAllocator>> OpenArrayScope(const key_type& key, size_t arraySize) {
		return OpenArrayScopeImpl(key, arraySize);
	}
	inline std::optional<RapidJsonArrayScope<TMode, TAllocator>> OpenArrayScope(const wchar_t* key, size_t arraySize) {
		return OpenArrayScopeImpl(key, arraySize);
	}

protected:
	template <typename TKey>
	bool SerializeBooleanImpl(TKey&& key, bool& value)
	{
		if constexpr (TMode == SerializeMode::Load)
		{
			auto* jsonValue = LoadJsonValue(key);
			if (jsonValue != nullptr && jsonValue->IsBool())
			{
				value = jsonValue->GetBool();
				return true;
			}
			return false;
		}
		else {
			return SaveJsonValue(std::forward<TKey>(key), RapidJsonNode(value));
		}
	}

	template <typename TKey, typename T, std::enable_if_t<std::is_fundamental_v<T>, int> = 0>
	bool SerializeFundamentalValueImpl(TKey&& key, T& value)
	{
		if constexpr (TMode == SerializeMode::Load)
		{
			auto* jsonValue = LoadJsonValue(key);
			return jsonValue == nullptr ? false : LoadFundamentalValue(*jsonValue, value);
		}
		else {
			return SaveJsonValue(std::forward<TKey>(key), RapidJsonNode(value));
		}
	}

	template <typename TKey, typename TSym, typename TStrAllocator>
	bool SerializeStringImpl(TKey&& key, std::basic_string<TSym, std::char_traits<TSym>, TStrAllocator>& value)
	{
		if constexpr (TMode == SerializeMode::Load)
		{
			auto* jsonValue = LoadJsonValue(key);
			return jsonValue == nullptr ? false : LoadString(*jsonValue, value);
		}
		else {
			return SaveJsonValue(key, RapidJsonScopeBase::MakeRapidJsonNodeFromString(value, mAllocator));
		}
	}

	template <typename TKey>
	std::optional<RapidJsonObjectScope<TMode, TAllocator>> OpenObjectScopeImpl(TKey&& key)
	{
		if constexpr (TMode == SerializeMode::Load)
		{
			auto* jsonValue = LoadJsonValue(key);
			if (jsonValue != nullptr && jsonValue->IsObject())
				return std::make_optional<RapidJsonObjectScope<TMode, TAllocator>>(jsonValue, mAllocator, this, key);
			return std::nullopt;
		}
		else
		{
			SaveJsonValue(std::forward<TKey>(key), RapidJsonNode(rapidjson::kObjectType));
			auto& insertedMember = FindMember(key)->value;
			return std::make_optional<RapidJsonObjectScope<TMode, TAllocator>>(&insertedMember, mAllocator, this, key);
		}
	}

	template <typename TKey>
	std::optional<RapidJsonArrayScope<TMode, TAllocator>> OpenArrayScopeImpl(TKey&& key, size_t arraySize)
	{
		if constexpr (TMode == SerializeMode::Load)
		{
			auto* jsonValue = LoadJsonValue(key);
			if (jsonValue != nullptr && jsonValue->IsArray())
				return std::make_optional<RapidJsonArrayScope<TMode, TAllocator>>(jsonValue, mAllocator, this, key);
			return std::nullopt;
		}
		else
		{
			auto rapidJsonArray = RapidJsonNode(rapidjson::kArrayType);
			rapidJsonArray.Reserve(static_cast<rapidjson::SizeType>(arraySize), mAllocator);
			SaveJsonValue(std::forward<TKey>(key), std::move(rapidJsonArray));
			auto& insertedMember = FindMember(key)->value;
			return std::make_optional<RapidJsonArrayScope<TMode, TAllocator>>(&insertedMember, mAllocator, this, key);
		}
	}

	inline auto FindMember(const key_type& key) const {
		return mNode->GetObject().FindMember(key.c_str());
	}

	inline auto FindMember(const wchar_t* key) const {
		return mNode->GetObject().FindMember(key);
	}

	inline const RapidJsonNode* LoadJsonValue(const key_type& key) const
	{
		const auto& jObject = mNode->GetObject();
		auto it = jObject.FindMember(key.c_str());
		return it == jObject.MemberEnd() ? nullptr : &it->value;
	}

	inline const RapidJsonNode* LoadJsonValue(const wchar_t* key) const
	{
		const auto& jObject = mNode->GetObject();
		const auto it = jObject.FindMember(key);
		return it == jObject.MemberEnd() ? nullptr : &it->value;
	}

	inline bool SaveJsonValue(const key_type& key, RapidJsonNode&& jsonValue) const
	{
		// Checks that object was not saved previously under the same key
		assert(mNode->GetObject().FindMember(key.c_str()) == mNode->GetObject().MemberEnd());

		auto jsonKey = RapidJsonNode(key.data(), static_cast<rapidjson::SizeType>(key.size()), mAllocator);
		mNode->AddMember(jsonKey.Move(), jsonValue.Move(), mAllocator);
		return true;
	}

	inline bool SaveJsonValue(const wchar_t* key, RapidJsonNode&& jsonValue) const
	{
		// Checks that object was not saved previously under the same key
		assert(mNode->GetObject().FindMember(key) == mNode->GetObject().MemberEnd());

		mNode->AddMember(rapidjson::GenericStringRef<RapidJsonNode::Ch>(key), jsonValue.Move(), mAllocator);
		return true;
	}

	TAllocator& mAllocator;
};


/// <summary>
/// JSON root scope (can serialize one value, array or object without key)
/// </summary>
template <SerializeMode TMode>
class RapidJsonRootScope : public ArchiveScope<TMode>, public RapidJsonScopeBase
{
protected:
	using RapidJsonDocument = rapidjson::GenericDocument<rapidjson::UTF16<>>;

public:
	explicit RapidJsonRootScope(const wchar_t* inputStr)
		: RapidJsonScopeBase(&mRootJson)
		, mOutput(nullptr)
	{
		static_assert(TMode == SerializeMode::Load, "BitSerializer. This data type can be used only in 'Load' mode.");
		if (mRootJson.Parse(inputStr).HasParseError())
			throw SerializationException(SerializationErrorCode::ParsingError, rapidjson::GetParseError_En(mRootJson.GetParseError()));
	}

	explicit RapidJsonRootScope(const std::wstring& inputStr) : RapidJsonRootScope(inputStr.c_str()) {}

	explicit RapidJsonRootScope(std::wstring& outputStr)
		: RapidJsonScopeBase(&mRootJson)
		, mOutput(&outputStr)
	{
		static_assert(TMode == SerializeMode::Save, "BitSerializer. This data type can be used only in 'Save' mode.");
	}

	explicit RapidJsonRootScope(std::wistream& inputStream)
		: RapidJsonScopeBase(&mRootJson)
		, mOutput(nullptr)
	{
		static_assert(TMode == SerializeMode::Load, "BitSerializer. This data type can be used only in 'Load' mode.");
		rapidjson::WIStreamWrapper isw(inputStream);
		if (mRootJson.ParseStream(isw).HasParseError())
			throw SerializationException(SerializationErrorCode::ParsingError, rapidjson::GetParseError_En(mRootJson.GetParseError()));
	}

	explicit RapidJsonRootScope(std::wostream& outputStream)
		: RapidJsonScopeBase(&mRootJson)
		, mOutput(&outputStream)
	{
		static_assert(TMode == SerializeMode::Save, "BitSerializer. This data type can be used only in 'Save' mode.");
	}

	virtual ~RapidJsonRootScope()
	{
		Finish();
	}

	void SerializeValue(bool& value)
	{
		if constexpr (TMode == SerializeMode::Load)
		{
			if (mRootJson.IsBool())
				value = mRootJson.GetBool();
		}
		else
		{
			assert(mRootJson.IsNull());
			mRootJson.SetBool(value);
		}
	}

	template <typename T, std::enable_if_t<std::is_fundamental_v<T>, int> = 0>
	void SerializeValue(T& value)
	{
		if constexpr (TMode == SerializeMode::Load) {
			LoadFundamentalValue(mRootJson, value);
		}
		else
		{
			assert(mRootJson.IsNull());
			SaveFundamentalValue(mRootJson, value);
		}
	}

	template <typename TSym, typename TAllocator>
	void SerializeString(std::basic_string<TSym, std::char_traits<TSym>, TAllocator>& value)
	{
		if constexpr (TMode == SerializeMode::Load) {
			LoadString(mRootJson, value);
		}
		else
		{
			assert(mRootJson.IsNull());
			using TargetSymType = RapidJsonNode::EncodingType::Ch;
			if constexpr (std::is_same_v<TSym, TargetSymType>)
				mRootJson.SetString(value.data(), static_cast<rapidjson::SizeType>(value.size()), mRootJson.GetAllocator());
			else {
				const auto str = Convert::To<std::basic_string<TargetSymType, std::char_traits<TargetSymType>>>(value);
				mRootJson.SetString(str.data(), static_cast<rapidjson::SizeType>(str.size()), mRootJson.GetAllocator());
			}
		}
	}

	std::optional<RapidJsonArrayScope<TMode, RapidJsonDocument::AllocatorType>> OpenArrayScope(size_t arraySize)
	{
		if constexpr (TMode == SerializeMode::Load)
		{
			return mRootJson.IsArray()
				? std::make_optional<RapidJsonArrayScope<TMode, RapidJsonDocument::AllocatorType>>(&mRootJson, mRootJson.GetAllocator())
				: std::nullopt;
		}
		else
		{
			assert(mRootJson.IsNull());
			mRootJson.SetArray();
			mRootJson.Reserve(static_cast<rapidjson::SizeType>(arraySize), mRootJson.GetAllocator());
			return std::make_optional<RapidJsonArrayScope<TMode, RapidJsonDocument::AllocatorType>>(&mRootJson, mRootJson.GetAllocator());
		}
	}

	std::optional<RapidJsonObjectScope<TMode, RapidJsonDocument::AllocatorType>> OpenObjectScope()
	{
		if constexpr (TMode == SerializeMode::Load)	{
			return mRootJson.IsObject()
				? std::make_optional<RapidJsonObjectScope<TMode, RapidJsonDocument::AllocatorType>>(&mRootJson, mRootJson.GetAllocator())
				: std::nullopt;
		}
		else
		{
			assert(mRootJson.IsNull());
			mRootJson.SetObject();
			return std::make_optional<RapidJsonObjectScope<TMode, RapidJsonDocument::AllocatorType>>(&mRootJson, mRootJson.GetAllocator());
		}
	}

private:
	void Finish()
	{
		if constexpr (TMode == SerializeMode::Save)
		{
			std::visit([this](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, std::wstring*>) {
					using StringBuffer = rapidjson::GenericStringBuffer<rapidjson::UTF16<>>;
					StringBuffer buffer;
					rapidjson::Writer<StringBuffer, rapidjson::UTF16<>, rapidjson::UTF16<>> writer(buffer);
					mRootJson.Accept(writer);
					*arg = buffer.GetString();
				}
				else if constexpr (std::is_same_v<T, std::wostream*>)
				{
					rapidjson::WOStreamWrapper osw(*arg);
					rapidjson::Writer<rapidjson::WOStreamWrapper, rapidjson::UTF16<>, rapidjson::UTF16<>> writer(osw);
					mRootJson.Accept(writer);
				}
			}, mOutput);
			mOutput = nullptr;
		}
	}

	RapidJsonDocument mRootJson;
	std::variant<std::nullptr_t, std::wstring*, std::wostream*> mOutput;
};

} //namespace Detail


/// <summary>
/// Declaration of JSON archive
/// </summary>
using JsonArchive = MediaArchiveBase<
	Detail::RapidJsonArchiveTraits,
	Detail::RapidJsonRootScope<SerializeMode::Load>,
	Detail::RapidJsonRootScope<SerializeMode::Save>>;

}	// namespace BitSerializer::Json::RapidJson