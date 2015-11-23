#pragma once

#include "meta/metafwd.hpp"
#include <string>
#include "util/view.hpp"

struct JsonSerializer
{
	std::string serialize(meta::ConstMetaReference object) const;
	bool deserialize(meta::MetaReference to_fill, StringView<const char> json_text) const;
};
