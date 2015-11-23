#pragma once

#include <utility>
#include "util/view.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include "util/algorithm.hpp"
#include "util/reusable_storage.hpp"
#include <functional>

namespace monad
{
template<typename>
struct BindType;
}

template<typename SuccessType, typename FailureType>
struct SuccessOrFailure
{
	SuccessOrFailure(SuccessType success)
		: success(std::move(success)), state(Success)
	{
	}
	SuccessOrFailure(FailureType failure)
		: failure(std::move(failure)), state(Failure)
	{
	}
	SuccessOrFailure(const SuccessOrFailure & other)
		: state(other.state)
	{
		if (state == Success) new (&success) SuccessType(other.success);
		else new (&failure) FailureType(other.failure);
	}
	SuccessOrFailure(SuccessOrFailure && other)
		: state(other.state)
	{
		if (state == Success) new (&success) SuccessType(std::move(other.success));
		else new (&failure) FailureType(std::move(other.failure));
	}
	SuccessOrFailure & operator=(SuccessOrFailure other)
	{
		if (other.state == Failure) return *this = std::move(other.failure);
		else return *this = std::move(other.success);
	}
	SuccessOrFailure & operator=(FailureType other)
	{
		if (state == Failure) failure = std::move(other);
		else
		{
			SuccessType temp = std::move(success);
			success.~SuccessType();
			try
			{
				new (&failure) FailureType(std::move(other));
				state = Failure;
			}
			catch(...)
			{
				new (&success) SuccessType(std::move(temp));
				throw;
			}
		}
		return *this;
	}
	SuccessOrFailure & operator=(SuccessType other)
	{
		if (state == Success) success = std::move(other);
		else
		{
			FailureType temp = std::move(failure);
			failure.~FailureType();
			try
			{
				new (&success) SuccessType(std::move(other));
				state = Success;
			}
			catch(...)
			{
				new (&failure) FailureType(std::move(temp));
				throw;
			}
		}
		return *this;
	}
	~SuccessOrFailure()
	{
		if (state == Success) success.~SuccessType();
		else failure.~FailureType();
	}

	bool IsSuccess() const
	{
		return state == Success;
	}
	bool IsFailure() const
	{
		return state == Failure;
	}
	SuccessType && GetSuccess() &&
	{
		return std::move(success);
	}
	const SuccessType & GetSuccess() const &
	{
		return success;
	}
	FailureType && GetFailure() &&
	{
		return std::move(failure);
	}
	const FailureType & GetFailure() const &
	{
		return failure;
	}

	template<typename FailureFunction, typename SuccessFunction>
	auto Access(const FailureFunction & on_failure, const SuccessFunction & on_success) &
	{
		if (state == Failure) return on_failure(failure);
		else return on_success(success);
	}
	template<typename FailureFunction, typename SuccessFunction>
	auto Access(const FailureFunction & on_failure, const SuccessFunction & on_success) const &
	{
		if (state == Failure) return on_failure(failure);
		else return on_success(success);
	}
	template<typename FailureFunction, typename SuccessFunction>
	auto Access(const FailureFunction & on_failure, const SuccessFunction & on_success) &&
	{
		if (state == Failure) return on_failure(std::move(failure));
		else return on_success(std::move(success));
	}
	template<typename FailureFunction, typename SuccessFunction>
	auto RValueAccess(const FailureFunction & on_failure, const SuccessFunction & on_success)
	{
		if (state == Failure) return on_failure(std::move(failure));
		else return on_success(std::move(success));
	}

private:
	enum State
	{
		Success,
		Failure
	};

	union
	{
		SuccessType success;
		FailureType failure;
	};

	State state;
};

namespace monad
{
template<typename T, typename U>
struct BindType<SuccessOrFailure<T, U> >
{
	typedef T type;
};
}

template<typename SuccessType, typename FailureType, typename FunctionType, typename ResultType = typename std::result_of<FunctionType(SuccessType)>::type>
inline ResultType operator>>=(SuccessOrFailure<SuccessType, FailureType> && object, const FunctionType & call)
{
	if (object.IsFailure()) return std::move(object.GetFailure());
	else return call(std::move(object.GetSuccess()));
}
template<typename SuccessType, typename FailureType, typename FunctionType, typename ResultType = typename std::result_of<FunctionType(SuccessType)>::type>
inline ResultType operator>>=(const SuccessOrFailure<SuccessType, FailureType> & object, const FunctionType & call)
{
	if (object.IsFailure()) return object.GetFailure();
	else return call(object.GetSuccess());
}
template<typename FunctionType, typename SuccessType, typename FailureType, typename ResultType = typename std::result_of<FunctionType(SuccessType)>::type>
inline SuccessOrFailure<ResultType, FailureType> fmap(const FunctionType & function, SuccessOrFailure<SuccessType, FailureType> && object)
{
	return std::move(object) >>= [&](SuccessType && success)
	{
		return SuccessOrFailure<ResultType, FailureType>(function(std::move(success)));
	};
}
template<typename FunctionType, typename SuccessType, typename FailureType, typename ResultType = typename std::result_of<FunctionType(SuccessType)>::type>
inline SuccessOrFailure<ResultType, FailureType> fmap(const FunctionType & function, const SuccessOrFailure<SuccessType, FailureType> & object)
{
	return object >>= [&](SuccessType && success)
	{
		return SuccessOrFailure<ResultType, FailureType>(function(success));
	};
}

struct ParseState
{
    ParseState(StringView<const char> text, ReusableStorage<std::string> & string_storage)
		: text(text), string_storage(string_storage)
	{
	}
    ParseState(const ParseState & other, StringView<const char> new_text)
		: text(new_text), string_storage(other.string_storage)
	{
	}
    ParseState NewText(StringView<const char> text) const
	{
		return ParseState(*this, text);
	}

    StringView<const char> text;
	std::reference_wrapper<ReusableStorage<std::string>> string_storage;
};

struct ParseErrorMessage
{
	ParseErrorMessage(const char * message, ParseState error_state)
		: message(message), error_state(std::move(error_state))
	{
	}

	const char * c_str() const
	{
		return message;
	}

	static ParseErrorMessage append(ParseErrorMessage lhs, ParseErrorMessage rhs)
	{
		return lhs.error_state.text.begin() > rhs.error_state.text.begin() ? lhs : rhs;
	}

private:
	const char * message;
	ParseState error_state;
};

template<typename T>
struct ParseSuccess
{
	ParseSuccess(T result, ParseState new_state)
		: result(std::move(result)), new_state(std::move(new_state))
	{
	}
	ParseSuccess(ParseSuccess &&) = default;
	ParseSuccess(const ParseSuccess &) = delete;
	ParseSuccess & operator=(ParseSuccess &&) = default;
	ParseSuccess & operator=(const ParseSuccess &) = delete;

	T result;
	ParseState new_state;
};

template<>
struct ParseSuccess<void>
{
	explicit ParseSuccess(ParseState new_state)
		: new_state(std::move(new_state))
	{
	}
	ParseSuccess(ParseSuccess &&) = default;
	ParseSuccess(const ParseSuccess &) = delete; // these are deleted not because it's invalid to copy this struct, but because it's usually an error
	ParseSuccess & operator=(ParseSuccess &&) = default;
	ParseSuccess & operator=(const ParseSuccess &) = delete; // these are deleted not because it's invalid to copy this struct, but because it's usually an error

	ParseState new_state;
};

template<typename T>
struct ParseResult
{
	ParseResult(ParseSuccess<T> success)
		: result(std::move(success))
	{
	}
	ParseResult(ParseErrorMessage message)
		: result(std::move(message))
	{
	}
	ParseResult(ParseResult &&) = default;
	ParseResult(const ParseResult &) = delete; // these are deleted not because it's invalid to copy this struct, but because it's usually an error
	ParseResult & operator=(ParseResult &&) = default;
	ParseResult & operator=(const ParseResult &) = delete; // these are deleted not because it's invalid to copy this struct, but because it's usually an error

	const SuccessOrFailure<ParseSuccess<T>, ParseErrorMessage> & GetResult() const &
	{
		return result;
	}
	SuccessOrFailure<ParseSuccess<T>, ParseErrorMessage> && GetResult() &&
	{
		return std::move(result);
	}

	template<typename Function>
	auto thenKeepRhs(const Function & rhs) const
	{
		return result >>= [&](const ParseSuccess<T> & success)
		{
			return rhs(success.new_state);
		};
	}
	template<typename Function>
	ParseResult thenKeepLhs(const Function & rhs) &&
	{
		if (result.IsFailure()) return result.GetFailure();
		auto rhs_result = rhs(result.GetSuccess().new_state);
		if (rhs_result.GetResult().IsFailure()) return rhs_result.GetResult().GetFailure();
		ParseSuccess<T> success = std::move(result).GetSuccess();
		success.new_state = rhs_result.GetResult().GetSuccess().new_state;
		return std::move(success);
	}
	template<typename Function>
	ParseResult orParse(const Function & rhs, const ParseState & old_state) &&
	{
		return result.RValueAccess([&](const ParseErrorMessage &)
		{
			return rhs(old_state);
		},
		[](ParseSuccess<T> my_success) -> ParseResult
		{
			return std::move(my_success);
		});
	}
	ParseSuccess<T> orReturnDefault(const ParseSuccess<T> & default_success) &&
	{
		return result.RValueAccess([&](const ParseErrorMessage &)
		{
			return default_success;
		},
		[](ParseSuccess<T> success)
		{
			return success;
		});
	}
	ParseSuccess<T> orReturnDefault(ParseSuccess<T> && default_success) &&
	{
		return result.RValueAccess([&](const ParseErrorMessage &)
		{
			return std::move(default_success);
		},
		[](ParseSuccess<T> success)
		{
			return success;
		});
	}

private:
	SuccessOrFailure<ParseSuccess<T>, ParseErrorMessage> result;
};

namespace monad
{
template<typename T>
struct BindType<ParseResult<T> >
{
	typedef ParseSuccess<T> type;
};
}


template<typename T, typename FunctionType, typename ResultType = typename std::result_of<FunctionType(ParseSuccess<T>)>::type>
inline ResultType operator>>=(ParseResult<T> object, const FunctionType & call)
{
	if (object.GetResult().IsFailure()) return std::move(object).GetResult().GetFailure();
	else return call(std::move(object).GetResult().GetSuccess());
}
template<typename T, typename FunctionType, typename ResultType = typename std::result_of<FunctionType(T)>::type>
inline ParseResult<ResultType> fmap(ParseResult<T> object, const FunctionType & function)
{
	return std::move(object) >>= [&](ParseSuccess<T> success) -> ParseResult<ResultType>
	{
		return ParseSuccess<ResultType>(function(std::move(success.result)), std::move(success.new_state));
	};
}

inline ParseResult<StringView<const char> > skip_whitespace(ParseState state)
{
	auto end_whitespace = std::find_if(state.text.begin(), state.text.end(), [](char c)
	{
		switch(c)
		{
		case ' ':
		case '\t':
		case '\n':
		case '\r':
			return false;
		default:
			return true;
		}
	});
    return ParseSuccess<StringView<const char> >({ state.text.begin(), end_whitespace }, state.NewText({ end_whitespace, state.text.end() }));
}

inline const char * find_end_of_identifier(StringView<const char> view)
{
    auto check_identifier_begin = [](char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    };
    auto check_identifier_remainder = [&](char c)
    {
        return check_identifier_begin(c) || (c >= '0' && c <= '9');
    };

    if (view.empty() || !check_identifier_begin(view.front()))
        return view.begin();

    auto next = std::find_if(view.begin() + 1, view.end(), [&](char c){ return !check_identifier_remainder(c); });
    // check if it's a C++ namespace, for example foo::Bar
    if (next == view.end() || *next != ':')
        return next;
    auto next_next = next + 1;
    if (next_next == view.end() || *next_next != ':')
        return next;
    auto next_next_next = next_next + 1;
    if (next_next_next == view.end() || !check_identifier_begin(*next_next_next))
        return next;
    return find_end_of_identifier({ next_next_next, view.end() });
}

inline ParseResult<StringView<const char> > parse_identifier(ParseState state)
{
    auto end = find_end_of_identifier(state.text);
    if (end == state.text.begin())
        return ParseErrorMessage("expected an identifier", state);

    return ParseSuccess<StringView<const char> >({ state.text.begin(), end }, state.NewText({ end, state.text.end() }));
}

inline ParseResult<void> parse_char(ParseState state, char c)
{
	if (state.text.empty()) return ParseErrorMessage("unexpected end of text", state);
	else if (state.text[0] == c) return ParseSuccess<void>(state.NewText({ state.text.begin() + 1, state.text.end() }));
	else return ParseErrorMessage("unexpected character", state);
}
template<size_t Size>
inline ParseResult<void> parse_word(ParseState state, const char (&word)[Size])
{
    if (state.text.startswith(word)) return ParseSuccess<void>(state.NewText({ state.text.begin() + Size - 1, state.text.end() }));
	else return ParseErrorMessage("expected word", state);
}

template<size_t Size>
inline ParseResult<char> parse_one_of(ParseState state, const char (&chars)[Size])
{
	if (state.text.empty()) return ParseErrorMessage("unexpected end of text", state);
	auto chars_end = chars + Size - 1;
	auto found = std::find(chars, chars_end, state.text[0]);
	if (found == chars_end) return ParseErrorMessage("unexpected character", state);
	else return ParseSuccess<char>(*found, state.NewText({ state.text.begin() + 1, state.text.end() }));
}

inline bool is_digit(char c)
{
	return c >= '0' && c <= '9';
}
inline int digit_to_int(char c)
{
	return c - '0';
}

template<typename T>
ParseResult<T> parse_unsigned(ParseState state)
{
	auto end = std::find_if(state.text.begin(), state.text.end(), [](char c){ return !is_digit(c); });
	if (state.text.begin() == end) return ParseErrorMessage("error while trying to parse an int. expected a digit", state);
	T sum = 0;
    for (char character : StringView<const char>(state.text.begin(), end))
	{
		T sum_before = sum;
		sum *= 10;
		sum += digit_to_int(character);
		if (sum < sum_before) return ParseErrorMessage("integer overflow while parsing", state);
	}
	return ParseSuccess<T>(sum, state.NewText({ end, state.text.end() }));
}

template<typename T, typename U>
ParseResult<T> parse_checked_cast(ParseSuccess<U> success)
{
	T casted = static_cast<T>(success.result);
	if (static_cast<U>(casted) == success.result) return ParseSuccess<T>(casted, std::move(success.new_state));
	else return ParseErrorMessage("integer overflow while parsing", success.new_state);
}

template<typename T>
ParseResult<T> parse_signed(ParseState state)
{
	return parse_char(state, '-').GetResult().Access(
		[&](const ParseErrorMessage &) -> ParseResult<T>
	{
		return parse_unsigned<T>(state);
	},
		[](ParseSuccess<void> success) -> ParseResult<T>
	{
		return fmap(parse_unsigned<unsigned long long>(std::move(success.new_state))
					>>= &parse_checked_cast<long long, unsigned long long>,
					std::negate<long long>())
				>>= &parse_checked_cast<T, long long>;
	});
}

struct FloatParser
{
	template<typename T>
	static ParseResult<T> parse_float(ParseState state)
	{
		return (parse_char(state, '-') >>= [&](ParseSuccess<void> lhs)
		{
			return fmap(parse_without_sign<T>(std::move(lhs.new_state)), std::negate<T>());
		}).orParse(&parse_without_sign<T>, state);
	}

private:
	template<typename T>
	static ParseResult<T> parse_without_sign(ParseState state)
	{
		return parse_unsigned<uintmax_t>(state) >>= [](ParseSuccess<uintmax_t> before_comma_success) -> ParseResult<T>
		{
			ParseSuccess<long double> after_comma = parse_char(before_comma_success.new_state, '.')
					.thenKeepRhs(&parse_fraction)
					.orReturnDefault(ParseSuccess<long double>(0.0L, before_comma_success.new_state));
			ParseSuccess<int16_t> exponent = parse_one_of(after_comma.new_state, "eE")
					.thenKeepRhs(&parse_exponent)
					.orReturnDefault(ParseSuccess<int16_t>(0, after_comma.new_state));
			return ParseSuccess<T>(combine_float(before_comma_success.result, after_comma.result, exponent.result), std::move(exponent.new_state));
		};
	}

	static ParseResult<long double> parse_fraction(ParseState state);
	static ParseResult<int16_t> parse_exponent(ParseState state);
	static long double combine_float(uintmax_t sum_before_comma, long double fraction, int16_t exponent);
};

ParseResult<ReusableStorage<std::string>::Reusable> parse_string(ParseState state);
ParseResult<bool> parse_bool(ParseState state);

template<typename Function>
ParseResult<void> parse_zero_or_more(ParseState state, const Function & function)
{
	for (auto result = function(state); result.GetResult().IsSuccess(); result = function(state))
	{
		state = result.GetResult().GetSuccess().new_state;
	}
	return ParseSuccess<void>(state);
}
