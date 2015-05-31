#pragma once

#include "assert.hpp"
#include <functional>

namespace assert
{
std::function<AssertBreakPoint (const AssertContext &)> SetAssertCallback(std::function<AssertBreakPoint (const AssertContext &)>);
const std::function<AssertBreakPoint (const AssertContext &)> & GetAssertCallback();
struct ScopedSetAssertCallback
{
    ScopedSetAssertCallback(std::function<AssertBreakPoint (const AssertContext &)> callback)
		: old_callback(SetAssertCallback(std::move(callback)))
	{
	}
	~ScopedSetAssertCallback()
	{
		SetAssertCallback(std::move(old_callback));
	}

private:
    std::function<AssertBreakPoint (const AssertContext &)> old_callback;
};
}
