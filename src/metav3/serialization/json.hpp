#pragma once

#include "metav3/metav3fwd.hpp"
#include <string>
#include "util/view.hpp"

struct JsonSerializer
{
    std::string serialize(metav3::ConstMetaReference object) const;
    bool deserialize(metav3::MetaReference to_fill, StringView<const char> json_text) const;
};
