#pragma once

#include "meta/meta.hpp"
#include <array>
#include <deque>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "metaStlMap.hpp"

namespace meta
{

template<typename T, size_t Size>
struct MetaType::MetaTypeConstructor<std::array<T, Size>>
{
	static const MetaType type;
};
template<typename T, size_t Size>
const MetaType MetaType::MetaTypeConstructor<std::array<T, Size>>::type = MetaType::RegisterArray<std::array<T, Size>>(GetMetaType<T>(), Size);

template<typename T, typename A>
struct MetaType::MetaTypeConstructor<std::deque<T, A>>
{
	static const MetaType type;
};
template<typename T, typename A>
const MetaType MetaType::MetaTypeConstructor<std::deque<T, A>>::type = MetaType::RegisterList<std::deque<T, A>>();

template<typename T, typename C, typename A>
struct MetaType::MetaTypeConstructor<std::multiset<T, C, A>>
{
	static const MetaType type;
};
template<typename T, typename C, typename A>
const MetaType MetaType::MetaTypeConstructor<std::multiset<T, C, A>>::type = MetaType::RegisterSet<std::multiset<T, C, A>>();
template<typename T, typename C, typename A>
struct MetaType::SetInfo::Creator<std::multiset<T, C, A>>
{
	static SetInfo Create()
	{
		typedef std::multiset<T, C, A> Self;
		return
		{
			GetMetaType<T>(),
			[](const MetaReference & set) -> size_t
			{
				return set.Get<Self>().size();
			},
			[](const MetaReference & set, const MetaReference & key) -> std::pair<SetIterator, SetIterator>
			{
				return const_cast<Self &>(set.Get<Self>()).equal_range(key.Get<T>());
			},
			[](MetaReference & object, SetIterator it) -> SetIterator
			{
				return object.Get<Self>().erase(*it.target<typename Self::iterator>());
			},
			[](MetaReference & object, MetaReference && value) -> std::pair<SetIterator, bool>
			{
				return { object.Get<Self>().insert(std::move(value.Get<T>())), true };
			},
			[](const MetaReference & object) -> SetIterator
			{
				return const_cast<Self &>(object.Get<Self>()).begin();
			},
			[](const MetaReference & object) -> SetIterator
			{
				return const_cast<Self &>(object.Get<Self>()).end();
			}
		};
	}
};

template<typename F, typename S>
struct MetaType::MetaTypeConstructor<std::pair<F, S>>
{
	static const MetaType type;

private:
	static StructInfo::MemberCollection GetMembers(int)
	{
		return
		{
			{
				MetaMember::CreateFromMemPtr<F, std::pair<F, S>, &std::pair<F, S>::first>("first"),
				MetaMember::CreateFromMemPtr<S, std::pair<F, S>, &std::pair<F, S>::second>("second")
			},
			{}
		};
	}
};
template<typename F, typename S>
const MetaType MetaType::MetaTypeConstructor<std::pair<F, S>>::type = MetaType::RegisterStruct<std::pair<F, S>>("pair", 0, &MetaType::MetaTypeConstructor<std::pair<F, S>>::GetMembers);

template<typename T, typename C, typename A>
struct MetaType::MetaTypeConstructor<std::set<T, C, A>>
{
	static const MetaType type;
};
template<typename T, typename C, typename A>
const MetaType MetaType::MetaTypeConstructor<std::set<T, C, A>>::type = MetaType::RegisterSet<std::set<T, C, A>>();

template<typename T, typename H, typename E, typename A>
struct MetaType::MetaTypeConstructor<std::unordered_multiset<T, H, E, A>>
{
	static const MetaType type;
};
template<typename T, typename H, typename E, typename A>
const MetaType MetaType::MetaTypeConstructor<std::unordered_multiset<T, H, E, A>>::type = MetaType::RegisterSet<std::unordered_multiset<T, H, E, A>>();
template<typename T, typename H, typename E, typename A>
struct MetaType::SetInfo::Creator<std::unordered_multiset<T, H, E, A>>
{
	static SetInfo Create()
	{
		typedef std::unordered_multiset<T, H, E, A> Self;
		return
		{
			GetMetaType<T>(),
			[](const MetaReference & set) -> size_t
			{
				return set.Get<Self>().size();
			},
			[](const MetaReference & set, const MetaReference & key) -> std::pair<SetIterator, SetIterator>
			{
				return const_cast<Self &>(set.Get<Self>()).equal_range(key.Get<T>());
			},
			[](MetaReference & object, SetIterator it) -> SetIterator
			{
				return object.Get<Self>().erase(*it.target<typename Self::iterator>());
			},
			[](MetaReference & object, MetaReference && value) -> std::pair<SetIterator, bool>
			{
				return { object.Get<Self>().insert(std::move(value.Get<T>())), true };
			},
			[](const MetaReference & object) -> SetIterator
			{
				return const_cast<Self &>(object.Get<Self>()).begin();
			},
			[](const MetaReference & object) -> SetIterator
			{
				return const_cast<Self &>(object.Get<Self>()).end();
			}
		};
	}
};

template<typename K, typename V, typename H, typename E, typename A>
struct MetaType::MetaTypeConstructor<std::unordered_map<K, V, H, E, A>>
{
	static const MetaType type;
};
template<typename K, typename V, typename H, typename E, typename A>
const MetaType MetaType::MetaTypeConstructor<std::unordered_map<K, V, H, E, A>>::type = MetaType::RegisterMap<std::unordered_map<K, V, H, E, A>>();

template<typename K, typename V, typename H, typename E, typename A>
struct MetaType::MetaTypeConstructor<std::unordered_multimap<K, V, H, E, A>>
{
	static const MetaType type;
};
template<typename K, typename V, typename H, typename E, typename A>
const MetaType MetaType::MetaTypeConstructor<std::unordered_multimap<K, V, H, E, A>>::type = MetaType::RegisterMap<std::unordered_multimap<K, V, H, E, A>>();
template<typename K, typename V, typename H, typename E, typename A>
struct MetaType::MapInfo::Creator<std::unordered_multimap<K, V, H, E, A>>
{
	static MapInfo Create()
	{
		typedef std::unordered_multimap<K, V, H, E, A> Self;
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
                return { object.Get<Self>().emplace(std::move(key.Get<K>()), std::move(value.Get<V>())), true };
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

template<typename T, typename H, typename E, typename A>
struct MetaType::MetaTypeConstructor<std::unordered_set<T, H, E, A>>
{
	static const MetaType type;
};
template<typename T, typename H, typename E, typename A>
const MetaType MetaType::MetaTypeConstructor<std::unordered_set<T, H, E, A>>::type = MetaType::RegisterSet<std::unordered_set<T, H, E, A>>();

template<typename T, typename A>
struct MetaType::MetaTypeConstructor<std::vector<T, A>>
{
	static const MetaType type;
};
template<typename T, typename A>
const MetaType MetaType::MetaTypeConstructor<std::vector<T, A>>::type = MetaType::RegisterList<std::vector<T, A>>();

} // end namespace meta
