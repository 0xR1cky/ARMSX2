/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * Provides a map template which doesn't require heap allocations for lookups.
 */

#pragma once

#include "Pcsx2Defs.h"
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace detail
{
	struct transparent_string_hash
	{
		using is_transparent = void;

		std::size_t operator()(const std::string_view& v) const { return std::hash<std::string_view>{}(v); }
		std::size_t operator()(const std::string& s) const { return std::hash<std::string>{}(s); }
		std::size_t operator()(const char* s) const { return operator()(std::string_view(s)); }
	};

	struct transparent_string_equal
	{
		using is_transparent = void;

		bool operator()(const std::string& lhs, const std::string_view& rhs) const { return lhs == rhs; }
		bool operator()(const std::string& lhs, const std::string& rhs) const { return lhs == rhs; }
		bool operator()(const std::string& lhs, const char* rhs) const { return lhs == rhs; }
		bool operator()(const std::string_view& lhs, const std::string& rhs) const { return lhs == rhs; }
		bool operator()(const char* lhs, const std::string& rhs) const { return lhs == rhs; }
	};

	struct transparent_string_less
	{
		using is_transparent = void;

		bool operator()(const std::string& lhs, const std::string_view& rhs) const { return lhs < rhs; }
		bool operator()(const std::string& lhs, const std::string& rhs) const { return lhs < rhs; }
		bool operator()(const std::string& lhs, const char* rhs) const { return lhs < rhs; }
		bool operator()(const std::string_view& lhs, const std::string& rhs) const { return lhs < rhs; }
		bool operator()(const char* lhs, const std::string& rhs) const { return lhs < rhs; }
	};
} // namespace detail

// This requires C++20, so fallback to ugly heap allocations if we don't have it.
#if __cplusplus >= 202002L
template <typename ValueType>
using UnorderedStringMap =
	std::unordered_map<std::string, ValueType, detail::transparent_string_hash, detail::transparent_string_equal>;
template <typename ValueType>
using UnorderedStringMultimap =
	std::unordered_multimap<std::string, ValueType, detail::transparent_string_hash, detail::transparent_string_equal>;
using UnorderedStringSet =
	std::unordered_set<std::string, detail::transparent_string_hash, detail::transparent_string_equal>;
using UnorderedStringMultiSet =
	std::unordered_multiset<std::string, detail::transparent_string_hash, detail::transparent_string_equal>;

template <typename KeyType, typename ValueType>
__fi typename UnorderedStringMap<ValueType>::const_iterator
UnorderedStringMapFind(const UnorderedStringMap<ValueType>& map, const KeyType& key)
{
	return map.find(key);
}
template <typename KeyType, typename ValueType>
__fi typename UnorderedStringMap<ValueType>::iterator
UnorderedStringMapFind(UnorderedStringMap<ValueType>& map, const KeyType& key)
{
	return map.find(key);
}
template <typename KeyType, typename ValueType>
__fi typename UnorderedStringMultimap<ValueType>::const_iterator
UnorderedStringMultiMapFind(const UnorderedStringMultimap<ValueType>& map, const KeyType& key)
{
	return map.find(key);
}
template <typename KeyType, typename ValueType>
__fi std::pair<typename UnorderedStringMultimap<ValueType>::const_iterator, typename UnorderedStringMultimap<ValueType>::const_iterator>
UnorderedStringMultiMapEqualRange(const UnorderedStringMultimap<ValueType>& map, const KeyType& key)
{
	return map.equal_range(key);
}
template <typename KeyType, typename ValueType>
__fi typename UnorderedStringMultimap<ValueType>::iterator
UnorderedStringMultiMapFind(UnorderedStringMultimap<ValueType>& map, const KeyType& key)
{
	return map.find(key);
}
template <typename KeyType, typename ValueType>
__fi std::pair<typename UnorderedStringMultimap<ValueType>::iterator, typename UnorderedStringMultimap<ValueType>::iterator>
UnorderedStringMultiMapEqualRange(UnorderedStringMultimap<ValueType>& map, const KeyType& key)
{
	return map.equal_range(key);
}
#else
template <typename ValueType>
using UnorderedStringMap = std::unordered_map<std::string, ValueType>;
template <typename ValueType>
using UnorderedStringMultimap = std::unordered_multimap<std::string, ValueType>;
using UnorderedStringSet = std::unordered_set<std::string>;
using UnorderedStringMultiSet = std::unordered_multiset<std::string>;

template <typename KeyType, typename ValueType>
__fi typename UnorderedStringMap<ValueType>::const_iterator UnorderedStringMapFind(const UnorderedStringMap<ValueType>& map, const KeyType& key)
{
	return map.find(std::string(key));
}
template <typename KeyType, typename ValueType>
__fi typename UnorderedStringMap<ValueType>::iterator UnorderedStringMapFind(UnorderedStringMap<ValueType>& map, const KeyType& key)
{
	return map.find(std::string(key));
}
template <typename KeyType, typename ValueType>
__fi typename UnorderedStringMultimap<ValueType>::const_iterator UnorderedStringMultiMapFind(const UnorderedStringMultimap<ValueType>& map, const KeyType& key)
{
	return map.find(std::string(key));
}
template <typename KeyType, typename ValueType>
__fi std::pair<typename UnorderedStringMultimap<ValueType>::const_iterator, typename UnorderedStringMultimap<ValueType>::const_iterator>
UnorderedStringMultiMapEqualRange(const UnorderedStringMultimap<ValueType>& map, const KeyType& key)
{
	return map.equal_range(std::string(key));
}
template <typename KeyType, typename ValueType>
__fi typename UnorderedStringMultimap<ValueType>::iterator UnorderedStringMultiMapFind(UnorderedStringMultimap<ValueType>& map, const KeyType& key)
{
	return map.find(std::string(key));
}
template <typename KeyType, typename ValueType>
__fi std::pair<typename UnorderedStringMultimap<ValueType>::iterator, typename UnorderedStringMultimap<ValueType>::iterator>
UnorderedStringMultiMapEqualRange(UnorderedStringMultimap<ValueType>& map, const KeyType& key)
{
	return map.equal_range(std::string(key));
}
#endif

template <typename ValueType>
using StringMap = std::map<std::string, ValueType, detail::transparent_string_less>;
template <typename ValueType>
using StringMultiMap = std::multimap<std::string, ValueType, detail::transparent_string_less>;
using StringSet = std::set<std::string, detail::transparent_string_less>;
using StringMultiSet = std::multiset<std::string, detail::transparent_string_less>;
