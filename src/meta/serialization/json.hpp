#pragma once

#include "meta/metafwd.hpp"
#include <string>
#include "util/range.hpp"

struct JsonSerializer
{
	std::string serialize(meta::ConstMetaReference object) const;
	bool deserialize(meta::MetaReference to_fill, Range<const char> json_text) const;
};
