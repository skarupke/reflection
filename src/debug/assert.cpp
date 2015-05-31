#include "assert.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cerrno>
#include "assert_settings.hpp"

namespace assert
{
namespace detail
{
	static bool break_on_throw = true;
	static void empty_sigtrap_handler(int)
	{
	}
	BreakOnlyIfDebuggerAttached::BreakOnlyIfDebuggerAttached()
		: old_signal_handler(signal(SIGTRAP, &empty_sigtrap_handler))
	{
	}
	BreakOnlyIfDebuggerAttached::~BreakOnlyIfDebuggerAttached()
	{
		signal(SIGTRAP, old_signal_handler);
	}
	CLANG_ANALYZER_NORETURN AssertBreakPoint OnAssert(const AssertContext & context)
	{
        const std::function<AssertBreakPoint (const AssertContext &)> & assert_callback = GetAssertCallback();
		if (assert_callback) return assert_callback(context);
		else return ShouldBreak;
	}
}
ScopedSetBreakOnThrow::ScopedSetBreakOnThrow(bool value)
	: old_value(detail::break_on_throw)
{
	detail::break_on_throw = value;
}
ScopedSetBreakOnThrow::~ScopedSetBreakOnThrow()
{
	detail::break_on_throw = old_value;
}
bool ScopedSetBreakOnThrow::break_on_throw()
{
	return detail::break_on_throw;
}
}
