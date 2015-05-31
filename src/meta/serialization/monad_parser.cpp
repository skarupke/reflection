#if 0
#include "monad_parser.hpp"
#include <algorithm>
#include "Util/assert.hpp"

struct ParseError : public std::exception
{
	ParseError(const char * message, Range<const char>::iterator error_point)
		: message(message), error_point(error_point)
	{
	}

	virtual const char * what() const noexcept override
	{
		return message;
	}

private:
	const char * message;
	Range<const char>::iterator error_point;
};

ParseResult<Range<const char>> WhitespaceSkipper::operator()(Range<const char> text, MutableParseState &) const
{
	auto end_whitespace = std::find_if(text.begin(), text.end(), [](char c)
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
	return ParseSuccess<Range<const char>>({ text.begin(), end_whitespace }, { end_whitespace, text.end() });
}


inline static char unescape(Range<const char>::iterator it)
{
	switch (*it)
	{
	case '\'': return '\'';
	case '"': return '"';
	case '\\': return '\\';
	case 'n': return '\n';
	case 't': return '\t';
	default: DE_THROW(ParseError("couldn't parse escape sequence", it));
	}
}

ParseResult<StringStorage::ReusableString> StringParser::operator()(Range<const char> text, MutableParseState & state) const
{
	if (text.empty()) return ParseErrorMessage("unexpected end of text", text.begin());
	StringStorage::ReusableString result(state.string_storage.GetString());
	if (*text.begin() != '"') return ParseErrorMessage("expected string", text.begin());
	for (auto it = text.begin() + 1; it != text.end(); ++it)
	{
		switch(*it)
		{
		case '"':
			return ParseSuccess<StringStorage::ReusableString>(result, { it + 1, text.end() });
		case '\\':
		{
			++it;
			if (it == text.end()) return ParseErrorMessage("unexpected end of string", it);
			result.str.push_back(unescape(it));
			break;
		}
		case '\n':
			return ParseErrorMessage("newline character in string", it);
		default:
			result.str.push_back(*it);
			break;
		}
	}
	return ParseErrorMessage("there was a string without a closing \"", text.begin());
}


#ifndef DISABLE_GTEST
#include <gtest/gtest.h>
#include <iostream>

template<typename T>
void ASSERT_PARSE_RESULT(const T & expected, const ParseResult<T> & actual)
{
	actual.Access([&](const ParseErrorMessage & message)
	{
		std::cerr << message.c_str() << std::endl;
		ASSERT_TRUE(false);
	},
	[&](const ParseSuccess<T> & success)
	{
		ASSERT_EQ(expected, success.result);
	});
}

TEST(monad_parser, parse_ints)
{
	MutableParseState state;
	auto unsigned_parser = ParseUnsigned<unsigned>();
	ASSERT_PARSE_RESULT(123u, unsigned_parser("123", state));
	auto int_parser = ParseSigned<int>();
	ASSERT_PARSE_RESULT(-123, int_parser("-123", state));
	ASSERT_PARSE_RESULT(123, int_parser("123", state));
	ASSERT_FALSE(int_parser("-a", state).IsSuccess());
	auto int8_parser = ParseSigned<int8_t>();
	ASSERT_PARSE_RESULT(int8_t(127), int8_parser("127", state));
	ASSERT_PARSE_RESULT(int8_t(-128), int8_parser("-128", state));
	ASSERT_FALSE(int8_parser("128", state).IsSuccess());
	ASSERT_FALSE(int8_parser("1000", state).IsSuccess());
	ASSERT_FALSE(int8_parser("-129", state).IsSuccess());
	auto int64_parser = ParseSigned<int64_t>();
	ASSERT_PARSE_RESULT(int64_t(-9223372036854775807 - 1), int64_parser("-9223372036854775808", state));
}
void ASSERT_STRING_PARSE_RESULT(const std::string & expected, const ParseResult<StringStorage::ReusableString> & actual)
{
	actual.Access([&](const ParseErrorMessage &)
	{
		ASSERT_TRUE(false);
	},
	[&](const ParseSuccess<StringStorage::ReusableString> & success)
	{
		ASSERT_EQ(expected, success.result.str);
	});
}

TEST(monad_parser, parser_string)
{
	MutableParseState state;
	auto string_parser = ParseString();
	ASSERT_STRING_PARSE_RESULT("foo", string_parser(R"("foo")", state));
	ASSERT_STRING_PARSE_RESULT("f\n\"oo", string_parser(R"("f\n\"oo")", state));
}

TEST(monad_parser, keep_left)
{
	MutableParseState state;
	auto number_comma = CreateParser<int>(ParseSigned<int>() << ParseChar(','));
	ASSERT_FALSE(number_comma("5", state).IsSuccess());
	ASSERT_PARSE_RESULT(5, number_comma("5,", state));
	auto comma_seperated_list = ParseZeroOrMore(std::move(number_comma));
	ASSERT_PARSE_RESULT(std::vector<int>{}, comma_seperated_list("5", state));
	ASSERT_PARSE_RESULT(std::vector<int>{ 5 }, comma_seperated_list("5,6", state));
	ASSERT_PARSE_RESULT(std::vector<int>{ 5, 6, 7 }, comma_seperated_list("5,6,7,", state));
}

template<typename T>
struct NonCopyable
{
	NonCopyable(T object)
		: object(std::move(object))
	{
	}
	NonCopyable(const NonCopyable &) = delete;
	NonCopyable(NonCopyable &&) = default;
	NonCopyable & operator=(const NonCopyable &) = delete;
	NonCopyable & operator=(NonCopyable &&) = default;

	bool operator==(const NonCopyable & other) const
	{
		return object == other.object;
	}

	T object;
};

template<typename T>
NonCopyable<T> CreateNonCopyable(T object)
{
	return NonCopyable<T>(std::move(object));
}

template<typename T>
auto ParseVectorInt()
{
	return CreateParser<std::vector<NonCopyable<T> > >(
				ParseChar('[') >> SkipWhitespace()
				>> ParseWithSeparator(fmap(&CreateNonCopyable<T>, ParseSigned<T>()), SkipWhitespace() >> ParseChar(',') << SkipWhitespace())
				<< SkipWhitespace() << ParseChar(']')
	);
}

template<typename T>
auto ParseFourInts()
{
	return CreateParser<std::vector<NonCopyable<T> > >(
				ParseChar('[') >> SkipWhitespace()
				>> ParseNWithSeparator<4>(fmap(&CreateNonCopyable<T>, ParseSigned<T>()), SkipWhitespace() >> ParseChar(',') << SkipWhitespace())
				<< SkipWhitespace() << ParseChar(']'));
}

TEST(monad_parser, parser_vector_int)
{
	MutableParseState state;
	auto vector_parser = ParseVectorInt<int>();
	ASSERT_PARSE_RESULT(std::vector<NonCopyable<int>>(), vector_parser(R"([])", state));
	std::vector<NonCopyable<int>> onetwothree;
	onetwothree.emplace_back(1);
	onetwothree.emplace_back(2);
	onetwothree.emplace_back(3);
	ASSERT_PARSE_RESULT(onetwothree, vector_parser(R"([1, 2, 3])", state));
	ASSERT_FALSE(vector_parser(R"([1, 2,])", state).IsSuccess());
	auto four_parser = ParseFourInts<int>();
	ASSERT_FALSE(four_parser("[1, 2, 3]", state).IsSuccess());
	std::vector<NonCopyable<int>> onetofive;
	onetofive.emplace_back(1);
	onetofive.emplace_back(2);
	onetofive.emplace_back(3);
	onetofive.emplace_back(5);
	ASSERT_PARSE_RESULT(onetofive, four_parser(R"([1, 2, 3, 5])", state));
	ASSERT_FALSE(four_parser("[1, 2, 3, 5, 8]", state).IsSuccess());
}

#endif
#endif
