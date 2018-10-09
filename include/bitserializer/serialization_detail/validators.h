/*******************************************************************************
* Copyright (C) 2018 by Pavel Kisliak                                          *
* This file is part of BitSerializer library, licensed under the MIT license.  *
*******************************************************************************/
#pragma once
#include <string>
#include <optional>
#include "../string_conversion.h"

namespace BitSerializer
{
	/// <summary>
	/// Validates that field is deserialized.
	/// </summary>
	class Required
	{
	public:
		template <class TValue>
		std::optional<std::wstring> Validate(const TValue& value, bool isLoaded) noexcept
		{
			if (isLoaded)
				return std::nullopt;

			return L"This field is required";
		}
	};


	/// <summary>
	/// Validates that field is in required range.
	/// </summary>
	template <class TValue>
	class Range
	{
	public:
		Range(const TValue& min, const TValue& max)
			: mMin(min)
			, mMax(max)
		{
		}

		std::optional<std::wstring> Validate(const TValue& value, bool isLoaded)
		{
			if (value >= mMin && value < mMax)
				return std::nullopt;

			return L"Value must be between " + Convert::ToWString(mMin) + L" and " + Convert::ToWString(mMax);
		}

	private:
		TValue mMin;
		TValue mMax;
	};


	/// <summary>
	/// Validates that size of field (string, container) is greater or equal than specified value.
	/// </summary>
	class MinSize
	{
	public:
		MinSize(size_t minSize)
			: mMinSize(minSize)
		{
		}

		template <class TValue>
		std::optional<std::wstring> Validate(const TValue& value, bool isLoaded) noexcept
		{
			if constexpr (has_size_v<TValue>)
			{
				if (value.size() >= mMinSize)
					return std::nullopt;

				return L"The minimum size of this field should be " + Convert::ToWString(mMinSize) + L".";
			}
			else
			{
				static_assert(false, "BitSerializer. The 'MinSize' validator can be applied only for types which has size() method.");
			}
		}

	private:
		size_t mMinSize;
	};

	/// <summary>
	/// Validates that size of field (string, container) is not greater than specified value.
	/// </summary>
	class MaxSize
	{
	public:
		MaxSize(size_t maxSize)
			: mMaxSize(maxSize)
		{
		}

		template <class TValue>
		std::optional<std::wstring> Validate(const TValue& value, bool isLoaded) noexcept
		{
			if constexpr (has_size_v<TValue>)
			{
				if (value.size() < mMaxSize)
					return std::nullopt;

				return L"The maximum size of this field should be not greater than " + Convert::ToWString(mMaxSize) + L".";
			}
			else
			{
				static_assert(false, "BitSerializer. The 'MaxSize' validator can be applied only for types which has size() method.");
			}
		}

	private:
		size_t mMaxSize;
	};

}	// namespace BitSerializer