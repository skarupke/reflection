#pragma once

#include <map>
#include "metav3/metav3.hpp"

namespace metav3
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
            [](ConstMetaReference object) -> size_t
            {
                return object.Get<Self>().size();
            },
            [](MetaReference object, MetaReference && key, MetaReference && value) -> std::pair<MapIterator, bool>
            {
                return { object.Get<Self>().insert(std::make_pair(std::move(key.Get<K>()), std::move(value.Get<V>()))), true };
            },
            [](ConstMetaReference object) -> MapIterator
            {
                return const_cast<Self &>(object.Get<Self>()).begin();
            },
            [](ConstMetaReference object) -> MapIterator
            {
                return const_cast<Self &>(object.Get<Self>()).end();
            }
        };
    }
};
}
