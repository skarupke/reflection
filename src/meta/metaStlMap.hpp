#pragma once

#include <map>
#include "meta/meta.hpp"

namespace meta
{
template<typename K, typename V, typename C, typename A>
struct MetaType::MetaTypeConstructor<std::map<K, V, C, A>>
{
	static const MetaType type;
};
template<typename K, typename V, typename C, typename A>
const MetaType MetaType::MetaTypeConstructor<std::map<K, V, C, A>>::type = MetaType::RegisterMap<std::map<K, V, C, A>>();
template<typename K, typename V, typename C, typename A>
struct MetaType::MetaTypeConstructor<std::multimap<K, V, C, A>>
{
	static const MetaType type;
};
template<typename K, typename V, typename C, typename A>
const MetaType MetaType::MetaTypeConstructor<std::multimap<K, V, C, A>>::type = MetaType::RegisterMap<std::multimap<K, V, C, A>>();
template<typename K, typename V, typename C, typename A>
struct MetaType::MapInfo::Creator<std::multimap<K, V, C, A>>
{
	static MapInfo Create()
	{
		typedef std::multimap<K, V, C, A> Self;
		return
		{
			GetMetaType<K>(),
			GetMetaType<V>(),
			[](const MetaReference & object) -> size_t
			{
				return object.Get<Self>().size();
			},
			[](const MetaReference & object, const MetaReference & element) -> std::pair<MapIterator, MapIterator>
			{
				return const_cast<Self &>(object.Get<Self>()).equal_range(element.Get<K>());
			},
			[](MetaReference & object, MapIterator it) -> MapIterator
			{
				return object.Get<Self>().erase(*it.target<typename Self::iterator>());
			},
			[](MetaReference & object, MetaReference && key, MetaReference && value) -> std::pair<MapIterator, bool>
			{
				return { object.Get<Self>().insert(std::make_pair(std::move(key.Get<K>()), std::move(value.Get<V>()))), true };
			},
			[](const MetaReference & object) -> MapIterator
			{
				return const_cast<Self &>(object.Get<Self>()).begin();
			},
			[](const MetaReference & object) -> MapIterator
			{
				return const_cast<Self &>(object.Get<Self>()).end();
			}
		};
	}
};
}
