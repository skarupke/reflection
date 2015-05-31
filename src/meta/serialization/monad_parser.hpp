#pragma once

#if 0
#include <type_traits>
#include <utility>
#include <Util/range.hpp>
#include <vector>
#include <functional>
#include <algorithm>

namespace monad
{
	template<typename>
	struct BindType;

	template<typename T>
	struct Storer
	{
		T operator()(T obj) const
		{
			return std::move(obj);
		}
	};

	template<typename T>
	inline auto store(T && value)
	{
		return Storer<typename std::remove_reference<T>::type>()(std::forward<T>(value));
	}

	template<typename T>
	struct StoredToUsableCast
	{
		T operator()(T obj) const
		{
			return std::move(obj);
		}
	};

	template<typename T>
	inline auto stored_to_usable(T && value)
	{
		return StoredToUsableCast<typename std::remove_reference<T>::type>()(std::forward<T>(value));
	}
}

template<typename SuccessType, typename FailureType>
struct Either
{
	Either(SuccessType success)
		: success(std::move(success)), state(Success)
	{
	}
	Either(FailureType failure)
		: failure(std::move(failure)), state(Failure)
	{
	}
	Either(const Either & other)
		: state(other.state)
	{
		if (state == Success) new (&success) SuccessType(other.success);
		else new (&failure) FailureType(other.failure);
	}
	Either(Either && other)
		: state(other.state)
	{
		if (state == Success) new (&success) SuccessType(std::move(other.success));
		else new (&failure) FailureType(std::move(other.failure));
	}
	Either & operator=(Either other)
	{
		if (other.state == Failure) return *this = std::move(other.failure);
		else return *this = std::move(other.success);
	}
	Either & operator=(FailureType other)
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
	Either & operator=(SuccessType other)
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
	~Either()
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
	SuccessType & GetSuccess()
	{
		return success;
	}
	const SuccessType & GetSuccess() const
	{
		return success;
	}
	FailureType & GetFailure()
	{
		return failure;
	}
	const FailureType & GetFailure() const
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
template<typename SuccessType, typename FailureType>
struct BindType<Either<SuccessType, FailureType>>
{
	typedef SuccessType type;
};
}


template<typename SuccessType, typename FailureType, typename FunctionType, typename NewEitherType = typename std::result_of<FunctionType(SuccessType)>::type>
inline NewEitherType operator>>=(Either<SuccessType, FailureType> && object, const FunctionType & call)
{
	if (object.IsFailure()) return std::move(object.GetFailure());
	else return call(std::move(object.GetSuccess()));
}
template<typename SuccessType, typename FailureType, typename FunctionType, typename NewEitherType = typename std::result_of<FunctionType(SuccessType)>::type>
inline NewEitherType operator>>=(const Either<SuccessType, FailureType> & object, const FunctionType & call)
{
	if (object.IsFailure()) return object.GetFailure();
	else return call(object.GetSuccess());
}

template<typename SuccessType, typename FailureType>
inline Either<SuccessType, FailureType> operator^(Either<SuccessType, FailureType> && lhs, Either<SuccessType, FailureType> && rhs)
{
	if (lhs.IsFailure()) return std::move(rhs);
	else return std::move(lhs.GetSuccess());
}

struct ParseErrorMessage
{
	ParseErrorMessage(const char * message, Range<const char>::iterator error_point)
		: message(message), error_point(error_point)
	{
	}

	const char * c_str() const
	{
		return message;
	}

	static ParseErrorMessage append(ParseErrorMessage lhs, ParseErrorMessage rhs)
	{
		return lhs.error_point > rhs.error_point ? lhs : rhs;
	}

private:
	const char * message;
	Range<const char>::iterator error_point;
};

struct StringStorage
{
	struct ReusableString
	{
		ReusableString(std::string && str, StringStorage & owner)
			: str(std::move(str)), owner(owner)
		{
		}
		~ReusableString()
		{
			str.clear();
			owner.StoreString(std::move(str));
		}

		std::string str;
	private:
		StringStorage & owner;
	};


	void StoreString(std::string && str)
	{
		if (str.capacity()) storage.push_back(std::move(str));
	}

	ReusableString GetString()
	{
		if (storage.empty()) return ReusableString(std::string(), *this);
		else
		{
			ReusableString result(std::move(storage.back()), *this);
			storage.pop_back();
			return result;
		}
	}

private:
	std::vector<std::string> storage;
};

struct MutableParseState
{
	StringStorage string_storage;
};

template<typename T>
struct ParseSuccess
{
	ParseSuccess(T result, Range<const char> remaining_text)
		: result(std::move(result)), remaining_text(remaining_text)
	{
	}

	T result;
	Range<const char> remaining_text;
};

template<>
struct ParseSuccess<void>
{
	ParseSuccess(Range<const char> remaining_text)
		: remaining_text(remaining_text)
	{
	}

	Range<const char> remaining_text;
};

template<typename T>
using ParseResult = Either<ParseSuccess<T>, ParseErrorMessage>;

template<typename T, typename Impl>
struct Parser
{
	Parser(Impl impl)
		: impl(std::move(impl))
	{
	}
	Parser(const Parser &) = delete;
	Parser(Parser &&) = default;
	Parser & operator=(const Parser &) = delete;
	Parser & operator=(Parser &&) = default;

	ParseResult<T> operator()(Range<const char> text, MutableParseState & state) const &
	{
		return impl(text, state);
	}
	ParseResult<T> operator()(Range<const char> text, MutableParseState & state) &&
	{
		return std::move(impl)(text, state);
	}

private:
	Impl impl;
};
// intentionally not implemented. there is no reason to use one parser as the impl
// of another parser. just use it directly
template<typename T, typename U, typename Impl>
struct Parser<T, Parser<U, Impl>>;

template<typename T, typename Impl>
struct StoredParser
{
	explicit StoredParser(Parser<T, Impl> parser)
		: parser(std::move(parser))
	{
	}

	ParseResult<T> operator()(Range<const char> text, MutableParseState & state) const
	{
		return parser(text, state);
	}

private:
	Parser<T, Impl> parser;
};

// intentionally not implemented. there is no reason to use one parser as the impl
// of another parser. just use it directly
template<typename T, typename U, typename Impl>
struct Parser<T, StoredParser<U, Impl> >;

namespace monad
{
template<typename T, typename Impl>
struct BindType<Parser<T, Impl> >
{
	typedef T type;
};
template<typename T, typename Impl>
struct BindType<std::reference_wrapper<const StoredParser<T, Impl> > >
{
	typedef T type;
};
}

template<typename T, typename Impl>
inline Parser<T, Impl> CreateParser(Impl impl)
{
	return Parser<T, Impl>(std::move(impl));
}
template<typename T, typename Impl>
inline Parser<T, Impl> CreateParser(Parser<T, Impl> parser)
{
	return parser;
}
namespace monad
{
template<typename T, typename Impl>
struct Storer<Parser<T, Impl> >
{
	StoredParser<T, Impl> operator()(Parser<T, Impl> parser) const
	{
		return StoredParser<T, Impl>(std::move(parser));
	}
};

template<typename T, typename Impl>
struct StoredToUsableCast<StoredParser<T, Impl> >
{
	std::reference_wrapper<const StoredParser<T, Impl> > operator()(const StoredParser<T, Impl> & lhs) const
	{
		return lhs;
	}
};

template<typename T, typename Impl>
struct StoredToUsableCast<const StoredParser<T, Impl> >
{
	std::reference_wrapper<const StoredParser<T, Impl> > operator()(const StoredParser<T, Impl> & lhs) const
	{
		return lhs;
	}
};
}

// when you combine several parsers together, the generated template mess
// can really confuse the compiler, debugger and IDE. you can use the
// PARSER_COMPILER_WALL macro to reduce the templates down to a single lambda
// which is equally unreadable but at least the tools don't break. this has
// zero overhead
#define PARSER_COMPILER_WALL(T, ...) [&]\
	{\
		auto compiler_wall_parser__ = __VA_ARGS__;\
		return CreateParser<T>([compiler_wall_parser__ = std::move(compiler_wall_parser__)](Range<const char> text, MutableParseState & state)\
		{\
			return compiler_wall_parser__(text, state);\
		});\
	}()
// use this define to turn off the PARSER_COMPILER_WALL
//#define PARSER_COMPILER_WALL(T, ...) CreateParser<T>(__VA_ARGS__)

template<typename T>
struct ParserReturner
{
	ParserReturner(T value)
		: value(std::move(value))
	{
	}

	ParseResult<T> operator()(Range<const char> text, MutableParseState &) const &
	{
		return ParseSuccess<T>(value, text);
	}
	ParseResult<T> operator()(Range<const char> text, MutableParseState &) &&
	{
		return ParseSuccess<T>(std::move(value), text);
	}

private:
	T value;
};

template<typename T>
Parser<T, ParserReturner<T> > ParseReturn(T value)
{
	return CreateParser<T>(ParserReturner<T>(std::move(value)));
}

template<typename Function, typename T, typename Impl>
auto fmap(Function function, Parser<T, Impl> parser)
{
	typedef typename std::result_of<Function(T)>::type ResultType;
	return CreateParser<ResultType>([function = std::move(function), parser = std::move(parser)](Range<const char> text, MutableParseState & state)
	{
		return parser(text, state) >>= [&](ParseSuccess<T> && success) -> ParseResult<ResultType>
		{
			return ParseSuccess<ResultType>(function(std::move(success.result)), success.remaining_text);
		};
	});
}

template<typename Function, typename T, typename Impl>
auto fmap(Function function, std::reference_wrapper<const StoredParser<T, Impl> > parser)
{
	typedef typename std::result_of<Function(T)>::type ResultType;
	return CreateParser<ResultType>([function = std::move(function), parser](Range<const char> text, MutableParseState & state)
	{
		return parser(text, state) >>= [&](ParseSuccess<T> && success) -> ParseResult<ResultType>
		{
			return ParseSuccess<ResultType>(function(std::move(success.result)), success.remaining_text);
		};
	});
}

template<typename T, typename Impl, typename Function, typename ResultType = typename monad::BindType<typename std::result_of<Function(T)>::type>::type>
inline auto operator>>=(Parser<T, Impl> parser, Function function)
{
	/*auto mapped = fmap(std::move(function), std::move(parser));
	return CreateParser<ResultType>([mapped = std::move(mapped)](Range<const char> text, MutableParseState & state)
	{
		return mapped(text, state) >>= [&state](ParseSuccess<typename std::result_of<Function(T)>::type> && success)
		{
			return std::move(success.result)(success.remaining_text, state);
		};
	});*/
	return CreateParser<ResultType>([parser = std::move(parser), function = std::move(function)](Range<const char> text, MutableParseState & state)
	{
		return parser(text, state) >>= [&](ParseSuccess<T> && success) -> ParseResult<ResultType>
		{
			return function(std::move(success.result))(success.remaining_text, state);
		};
	});
}

template<typename T, typename Impl, typename Function, typename ResultType = typename monad::BindType<typename std::result_of<Function(T)>::type>::type>
inline auto operator>>=(std::reference_wrapper<const StoredParser<T, Impl> > parser, Function function)
{
	/*auto mapped = fmap(std::move(function), parser);
	return CreateParser<ResultType>([mapped = std::move(mapped)](Range<const char> text, MutableParseState & state)
	{
		return mapped(text, state) >>= [&state](ParseSuccess<typename std::result_of<Function(T)>::type> && success)
		{
			return std::move(success.result)(success.remaining_text, state);
		};
	});*/
	return CreateParser<ResultType>([parser = monad::store(std::move(parser)), function = std::move(function)](Range<const char> text, MutableParseState & state)
	{
		return monad::stored_to_usable(parser)(text, state) >>= [&](ParseSuccess<T> && success) -> ParseResult<ResultType>
		{
			return function(std::move(success.result))(success.remaining_text, state);
		};
	});
}

template<typename T, typename U, typename Function>
inline auto liftM2(T lhs, U rhs, Function function)
{
	using namespace monad;
	return std::move(lhs) >>= [rhs = store(std::move(rhs)), function = std::move(function)](typename BindType<T>::type a_result)
	{
		return stored_to_usable(rhs) >>= [&, a_result = std::move(a_result)](typename BindType<U>::type b_result) mutable
		{
			return ParseReturn(function(std::move(a_result), std::move(b_result)));
		};
	};
}

template<typename T, typename U, typename ResultType = typename monad::BindType<T>::type, typename IgnoreType = typename monad::BindType<U>::type>
auto operator<<(T lhs, U rhs)
{
	using namespace monad;
	return liftM2(std::move(lhs), std::move(rhs), [](ResultType lhs, const IgnoreType &)
	{
		return lhs;
	});
}

template<typename T, typename U, typename IgnoreType = typename monad::BindType<T>::type, typename ResultType = typename monad::BindType<U>::type>
auto operator>>(T lhs, U rhs)
{
	using namespace monad;
	return liftM2(std::move(lhs), std::move(rhs), [](const IgnoreType &, ResultType rhs)
	{
		return rhs;
	});
}

template<typename T, typename ImplA, typename ImplB>
auto operator^(Parser<T, ImplA> lhs, Parser<T, ImplB> rhs)
{
	return CreateParser<T>([lhs = std::move(lhs), rhs = std::move(rhs)](Range<const char> text, MutableParseState & state)
	{
		return lhs(text, state).Access([&](const ParseErrorMessage &) -> ParseResult<T>
		{
			return rhs(text, state);
		},
		[](ParseSuccess<T> success) -> ParseResult<T>
		{
			return success;
		});
	});
}

template<typename T, typename Impl, typename Function>
auto ParseOptional(Parser<T, Impl> parser, Function default_generator)
{
	return CreateParser<T>([parser = StoredParser<T, Impl>(std::move(parser)), default_generator = std::move(default_generator)](Range<const char> text, MutableParseState & state)
	{
		return parser(text, state) ^ ParseResult<T>(ParseSuccess<T>(default_generator(), text));
	});
}


struct WhitespaceSkipper
{
	ParseResult<Range<const char> > operator()(Range<const char> text, MutableParseState &) const;
};

inline Parser<Range<const char>, WhitespaceSkipper> SkipWhitespace()
{
	return CreateParser<Range<const char> >(WhitespaceSkipper());
}

template<typename T, typename ResultType = typename monad::BindType<T>::type>
auto ParseInto(T parser, std::vector<ResultType> & to_fill)
{
	return CreateParser<void>([parser = std::move(parser), &to_fill](Range<const char> text, MutableParseState & state)
	{
		for (ParseResult<ResultType> single_parse_result = parser(text, state); single_parse_result.IsSuccess(); single_parse_result = std::move(single_parse_result) >>= [&](ParseSuccess<ResultType> && success)
		{
			text = success.remaining_text;
			to_fill.push_back(std::move(success.result));
			return parser(text, state);
		});
		return ParseSuccess<void>(text);
	});
}

template<typename T, typename Result>
auto ParseThenReturnOther(T parser, Range<const char> text, MutableParseState & state, Result & result)
{
	std::move(parser)(text, state).Access([](const ParseErrorMessage &){}, [&](const ParseSuccess<typename monad::BindType<T>::type> & success)
	{
		text = success.remaining_text;
	});
	return ParseSuccess<Result>(std::move(result), text);
}

template<typename T, typename Impl>
auto ParseZeroOrMore(Parser<T, Impl> parser)
{
	return CreateParser<std::vector<T> >([parser = monad::store(std::move(parser))](Range<const char> text, MutableParseState & state)
	{
		std::vector<T> result;
		return ParseThenReturnOther(ParseInto(monad::stored_to_usable(parser), result), text, state, result);
	});
}

template<typename T, typename Impl, typename Separator, typename SeparatorImpl>
auto ParseWithSeparator(Parser<T, Impl> parser, Parser<Separator, SeparatorImpl> separator)
{
	using namespace monad;
	return CreateParser<std::vector<T> >([parser = StoredParser<T, Impl>(std::move(parser)), separator = StoredParser<Separator, SeparatorImpl>(std::move(separator))](Range<const char> text, MutableParseState & state)
	{
		std::vector<T> result;
		return ParseThenReturnOther(monad::stored_to_usable(parser) >>= [&](T value)
		{
			result.push_back(std::move(value));
			return ParseInto(monad::stored_to_usable(separator) >> monad::stored_to_usable(parser), result);
		}, text, state, result);
	});
}
template<unsigned N, typename T, typename Impl, typename Separator, typename SeparatorImpl>
auto ParseNWithSeparator(Parser<T, Impl> parser, Parser<Separator, SeparatorImpl> separator)
{
	using namespace monad;
	return CreateParser<std::vector<T> >([parser = StoredParser<T, Impl>(std::move(parser)), separator = StoredParser<Separator, SeparatorImpl>(std::move(separator))](Range<const char> text, MutableParseState & state) -> ParseResult<std::vector<T> >
	{
		std::vector<T> result;
		if (!N) return ParseSuccess<std::vector<T> >(std::move(result), text);
		auto parse_one = stored_to_usable(parser) << stored_to_usable(separator);
		for (unsigned i = 1; i < N; ++i)
		{
			ParseResult<T> single_parse = parse_one(text, state);
			if (single_parse.IsFailure()) return single_parse.GetFailure();
			result.push_back(std::move(single_parse.GetSuccess().result));
			text = single_parse.GetSuccess().remaining_text;
		}
		return parser(text, state) >>= [&](ParseSuccess<T> && success) -> ParseResult<std::vector<T> >
		{
			result.push_back(std::move(success.result));
			return ParseSuccess<std::vector<T> >(std::move(result), success.remaining_text);
		};
	});
}

inline auto ParseChar(char c)
{
	return CreateParser<char>([c](Range<const char> text, MutableParseState &) -> ParseResult<char>
	{
		if (text.empty()) return ParseErrorMessage("unexpected end of text", text.begin());
		else if (text[0] == c) return ParseSuccess<char>(c, { text.begin() + 1, text.end() });
		else return ParseErrorMessage("unexpected character", text.begin());
	});
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
auto ParseUnsigned()
{
	return CreateParser<T>([](Range<const char> text, MutableParseState &) -> ParseResult<T>
	{
		auto end = std::find_if(text.begin(), text.end(), [](char c){ return !is_digit(c); });
		if (text.begin() == end) return ParseErrorMessage("error while trying to parse an int. expected a digit", text.begin());
		T sum = 0;
		for (char character : Range<const char>(text.begin(), end))
		{
			T sum_before = sum;
			sum *= 10;
			sum += digit_to_int(character);
			if (sum < sum_before) return ParseErrorMessage("integer overflow while parsing", text.begin());
		}
		return ParseSuccess<T>(sum, { end, text.end() });
	});
}

template<typename T, typename U>
struct CheckedParseCaster
{
private:
	struct Impl
	{
		Impl(U value)
			: value(value)
		{
		}

		ParseResult<T> operator()(Range<const char> text, MutableParseState &) const
		{
			T casted = static_cast<T>(value);
			if (static_cast<U>(casted) == value) return ParseSuccess<T>(casted, text);
			else return ParseErrorMessage("integer overflow while parsing", text.begin());
		}

	private:
		U value;
	};

public:
	Parser<T, Impl> operator()(U value) const
	{
		return CreateParser<T>(Impl(value));
	}
};

template<typename T>
auto ParseSigned()
{
	return PARSER_COMPILER_WALL(T, (
		fmap
		(
			std::negate<long long>(),
			(ParseChar('-') >> SkipWhitespace() >> ParseUnsigned<unsigned long long>()) >>= CheckedParseCaster<long long, unsigned long long>()
		)
		>>= CheckedParseCaster<T, long long>()
	)
	^ ParseUnsigned<T>());
}

struct StringParser
{
	ParseResult<StringStorage::ReusableString> operator()(Range<const char> text, MutableParseState & state) const;
};

inline Parser<StringStorage::ReusableString, StringParser> ParseString()
{
	return CreateParser<StringStorage::ReusableString>(StringParser());
}
#endif
