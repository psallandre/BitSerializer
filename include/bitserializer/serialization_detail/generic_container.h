/*******************************************************************************
* Copyright (C) 2018-2021 by Pavel Kisliak                                     *
* This file is part of BitSerializer library, licensed under the MIT license.  *
*******************************************************************************/
#pragma once
#include "object_traits.h"
#include "archive_traits.h"

namespace BitSerializer::Detail
{
	/// <summary>
	/// Generic function for serialization containers with key.
	/// </summary>
	/// <returns>Returns <c>true</c> when value successfully loaded</returns>
	template<typename TArchive, typename TKey, typename TContainer>
	static void SerializeContainer(TArchive& arrayScope, TKey&& key, TContainer& cont)
	{
		constexpr auto hasArrayWithKeySupport = can_serialize_array_with_key_v<TArchive, TKey>;
		static_assert(hasArrayWithKeySupport, "BitSerializer. The archive doesn't support serialize array with key on this level.");

		if constexpr (hasArrayWithKeySupport)
		{
			if constexpr (TArchive::IsLoading() && is_resizeable_cont_v<TContainer>) {
				cont.resize(arrayScope.GetSize());
			}
			for (auto& elem : cont) {
				Serialize(arrayScope, elem);
			}
		}
	}

	/// <summary>
	/// Generic function for serialization containers.
	/// </summary>
	template<typename TArchive, typename TContainer>
	static void SerializeContainer(TArchive& arrayScope, TContainer& cont)
	{
		constexpr auto hasArraySupport = can_serialize_array_v<TArchive>;
		static_assert(hasArraySupport, "BitSerializer. The archive doesn't support serialize array without key on this level.");

		if constexpr (hasArraySupport)
		{
			if constexpr (TArchive::IsLoading() && is_resizeable_cont_v<TContainer>) {
				cont.resize(arrayScope.GetSize());
			}
			for (auto& elem : cont) {
				Serialize(arrayScope, elem);
			}
		}
	}

	/// <summary>
	/// Returns reference to internal container which is used as base for a container adapter such as std::queue.
	/// </summary>
	template <class TContainer>
	typename TContainer::container_type& GetBaseContainer(TContainer& container)
	{
		struct Accessor : TContainer
		{
			static auto& Get(TContainer& cont) {
				return cont.*(&Accessor::c);
			}
		};
		return Accessor::Get(container);
	}

	/// <summary>
	/// Returns constant reference to internal container which is used as base for a container adapter such as std::queue.
	/// </summary>
	template <class TContainer>
	const typename TContainer::container_type& GetBaseContainer(const TContainer& container)
	{
		struct Accessor : TContainer
		{
			static const auto& Get(const TContainer& cont) {
				return cont.*(&Accessor::c);
			}
		};
		return Accessor::Get(container);
	}
}
