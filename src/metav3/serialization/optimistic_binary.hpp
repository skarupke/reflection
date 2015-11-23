#pragma once

#include <iosfwd>
#include "metav3/metav3.hpp"
#include <string>

struct OptimisticBinarySerializer
{
    std::string serialize(metav3::ConstMetaReference object) const;
    bool deserialize(metav3::MetaReference object, const std::string & str) const;
};
void write_optimistic_binary(metav3::ConstMetaReference object, std::ostream & out);
void read_optimistic_binary(metav3::MetaReference object, ArrayView<const unsigned char> in);

