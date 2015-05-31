#pragma once

#include <iosfwd>
#include "meta/metafwd.hpp"
#include <string>

struct BinarySerializer
{
	std::string serialize(meta::ConstMetaReference object) const;
	bool deserialize(meta::MetaReference object, const std::string & str) const;
};
void write_binary(meta::ConstMetaReference object, std::ostream & out);
void read_binary(meta::MetaReference object, std::istream & in);
