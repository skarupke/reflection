#pragma once

#include "debug/assert.hpp"

#define UNHANDLED_ERROR(description, ...) RAW_ASSERT(false, description, __VA_ARGS__)
#define UNHANDLED_ERROR_THAT_COULD_BE_HANDLED(...) UNHANDLED_ERROR(__VA_ARGS__)

#define WARN_UNUSED_RESULT __attribute__ ((warn_unused_result))
