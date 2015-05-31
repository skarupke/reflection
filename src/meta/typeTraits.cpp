#include "typeTraits.hpp"
#include <string>

static_assert(meta::is_copy_constructible<std::string>::value, "copy constructible test");
static_assert(!meta::is_copy_constructible<std::unique_ptr<std::string>>::value, "copy constructible test");
static_assert(!meta::is_copy_constructible<std::vector<std::unique_ptr<std::string>>>::value, "copy constructible test");

