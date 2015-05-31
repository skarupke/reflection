#pragma once

#define DEBUG_BREAK() do { __asm__("int $3"); } while (false)
#define DEBUG_BREAK_IF_DEBUGGER_ATTACHED() do { ::assert::detail::BreakOnlyIfDebuggerAttached break_guard; DEBUG_BREAK(); } while (false)
namespace assert
{
enum AssertBreakPoint
{
	ShouldBreakIfDebuggerAttached,
	ShouldBreak,
	ShouldNotBreak
};
struct AssertContext
{
	const char * file;
	int line;
	const char * condition;
};

namespace detail
{
#ifndef CLANG_ANALYZER_NORETURN
#	ifdef __has_feature
#		if __has_feature(attribute_analyzer_noreturn)
#			define CLANG_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
#		else
#			define CLANG_ANALYZER_NORETURN
#		endif
#	else
#		define CLANG_ANALYZER_NORETURN
#	endif
#endif

	CLANG_ANALYZER_NORETURN AssertBreakPoint OnAssert(const AssertContext & context);

	struct BreakOnlyIfDebuggerAttached
	{
		BreakOnlyIfDebuggerAttached();
		~BreakOnlyIfDebuggerAttached();
	private:
		void (*old_signal_handler)(int);
	};
}
struct ScopedSetBreakOnThrow
{
	ScopedSetBreakOnThrow(bool value);
	~ScopedSetBreakOnThrow();
	static bool break_on_throw();
private:
	bool old_value;
};
}

#define UNLIKELY(x) __builtin_expect(static_cast<bool>(x), false)

#ifdef FINAL
#	define RAW_ASSERT(condition, ...) static_cast<void>(0)
#else
#	define RAW_ASSERT(condition, ...) \
    if (UNLIKELY(!(condition)))\
	{\
		::assert::AssertContext context = { __FILE__, __LINE__, #condition };\
		::assert::AssertBreakPoint should_break = ::assert::detail::OnAssert(context);\
		if (should_break == ::assert::ShouldBreakIfDebuggerAttached)\
			DEBUG_BREAK_IF_DEBUGGER_ATTACHED();\
		else if (should_break == ::assert::ShouldBreak)\
			DEBUG_BREAK();\
	}\
	else\
		static_cast<void>(0)
#endif

#ifdef FINAL
#	define RAW_VERIFY(condition, ...) if (condition) static_cast<void>(0); else static_cast<void>(0)
#else
#	define RAW_VERIFY(condition, ...) RAW_ASSERT(condition, __VA_ARGS__)
#endif

#ifdef FINAL
#	define RAW_THROW(exception) throw exception
#else
#	define RAW_THROW(exception) do { if (::assert::ScopedSetBreakOnThrow::break_on_throw()) DEBUG_BREAK_IF_DEBUGGER_ATTACHED(); throw exception; } while (false)
#endif

#define CHECK_FOR_PROGRAMMER_ERROR(condition, ...) RAW_ASSERT(condition, __VA_ARGS__)
#define CHECK_FOR_INVALID_DATA(condition, ...) RAW_ASSERT(condition, __VA_ARGS__)
