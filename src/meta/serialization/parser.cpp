#include "parser.hpp"

ParseResult<long double> FloatParser::parse_fraction(ParseState state)
{
	return parse_unsigned<uintmax_t>(state) >>= [&](ParseSuccess<uintmax_t> success) -> ParseResult<long double>
	{
		long double fraction = success.result * std::pow(0.1L, std::distance(state.text.begin(), success.new_state.text.begin()));//power_nonnegative(0.1L, std::distance(state.text.begin(), success.new_state.text.begin()), std::multiplies<long double>());
		return ParseSuccess<long double>(fraction, std::move(success.new_state));
	};
}
ParseResult<int16_t> FloatParser::parse_exponent(ParseState state)
{
	return parse_char(state, '+')
			.thenKeepRhs(&parse_unsigned<int16_t>)
			.orParse(&parse_signed<int16_t>, state);
}
long double FloatParser::combine_float(uintmax_t sum_before_comma, long double fraction, int16_t exponent)
{
	long double multiplier = std::pow(0.1L, std::max(0, int(-exponent))) * std::pow(10.0L, std::max(0, int(exponent)));//power(0.1L, std::max(0, int(-exponent)), std::multiplies<long double>(), 1.0L) * power(10.0L, std::max(0, int(exponent)), std::multiplies<long double>(), 1.0L);
	return sum_before_comma * multiplier + fraction * multiplier;
}

ParseResult<ReusableStorage<std::string>::Reusable> parse_string(ParseState state)
{
	if (state.text.empty() || state.text[0] != '"') return ParseErrorMessage("expected string literal", state);
	ReusableStorage<std::string>::Reusable result = state.string_storage.get().GetObject();
	for (auto it = state.text.begin() + 1; it != state.text.end(); ++it)
	{
		switch(*it)
		{
		case '"':
			return ParseSuccess<ReusableStorage<std::string>::Reusable>(std::move(result), state.NewText({ it + 1, state.text.end() }));
		case '\\':
		{
			++it;
			if (it == state.text.end()) return ParseErrorMessage("unexpected end of string", state.NewText({ it, state.text.end() }));
			switch(*it)
			{
			case '\'':
				result.object.push_back('\'');
				break;
			case '"':
				result.object.push_back('"');
				break;
			case '\\':
				result.object.push_back('\\');
				break;
			case 'n':
				result.object.push_back('\n');
				break;
			case 't':
				result.object.push_back('\t');
				break;
			default:
				return ParseErrorMessage("invalid escape character", state.NewText({ it, state.text.end() }));
			}
			break;
		}
		case '\n':
			ParseErrorMessage("newline in string literal", state.NewText({ it, state.text.end() }));
			break;
		default:
			result.object.push_back(*it);
			break;
		}
	}
	return ParseErrorMessage("there was a string sequence without a closing \"", state);
}

ParseResult<bool> parse_bool(ParseState state)
{
    if (state.text.startswith("true"))
	{
		return ParseSuccess<bool>(true, state.NewText({ state.text.begin() + 4, state.text.end() }));
	}
    else if (state.text.startswith("false"))
	{
		return ParseSuccess<bool>(false, state.NewText({ state.text.begin() + 5, state.text.end() }));
	}
	else return ParseErrorMessage("couldn't read boolean value. expected 'true' or 'false'", state);
}


#ifndef DISABLE_GTEST
#include <gtest/gtest.h>
#include <iostream>

template<typename T>
void ASSERT_PARSE_RESULT(const T & expected, const ParseResult<T> & actual)
{
	actual.GetResult().Access([&](const ParseErrorMessage & message)
	{
		std::cerr << message.c_str() << std::endl;
		ASSERT_TRUE(false);
	},
	[&](const ParseSuccess<T> & success)
	{
		ASSERT_EQ(expected, success.result);
	});
}

TEST(parser, parse_ints)
{
	ReusableStorage<std::string> storage;
	ParseState state("", storage);
	ASSERT_PARSE_RESULT(123u, parse_unsigned<unsigned>(state.NewText("123")));
	ASSERT_PARSE_RESULT(-123, parse_signed<int>(state.NewText("-123")));
	ASSERT_PARSE_RESULT(123, parse_signed<int>(state.NewText("123")));
	ASSERT_FALSE(parse_signed<int>(state.NewText("-a")).GetResult().IsSuccess());
	ASSERT_PARSE_RESULT(int8_t(127), parse_signed<int8_t>(state.NewText("127")));
	ASSERT_PARSE_RESULT(int8_t(-128), parse_signed<int8_t>(state.NewText("-128")));
	ASSERT_FALSE(parse_signed<int8_t>(state.NewText("128")).GetResult().IsSuccess());
	ASSERT_FALSE(parse_signed<int8_t>(state.NewText("1000")).GetResult().IsSuccess());
	ASSERT_FALSE(parse_signed<int8_t>(state.NewText("-129")).GetResult().IsSuccess());
	ASSERT_PARSE_RESULT(int64_t(-9223372036854775807 - 1), parse_signed<int64_t>(state.NewText("-9223372036854775808")));
}

TEST(parser, parser_floats)
{
	ReusableStorage<std::string> storage;
	ParseState state("", storage);
	ASSERT_PARSE_RESULT(float(123), FloatParser::parse_float<float>(state.NewText("123")));
	ASSERT_PARSE_RESULT(float(0.01), FloatParser::parse_float<float>(state.NewText("0.01")));
	ASSERT_PARSE_RESULT(float(0.01e100), FloatParser::parse_float<float>(state.NewText("0.01e100")));
	ASSERT_PARSE_RESULT(float(0.01e-55), FloatParser::parse_float<float>(state.NewText("0.01e-55")));
	ASSERT_PARSE_RESULT(float(-0.01e-55), FloatParser::parse_float<float>(state.NewText("-0.01e-55")));
	ASSERT_PARSE_RESULT(float(0.765521234527), FloatParser::parse_float<float>(state.NewText("0.765521234527")));
	ASSERT_PARSE_RESULT(float(-123458335.765521234527), FloatParser::parse_float<float>(state.NewText("-123458335.765521234527")));
}

#include <thread>
#include <math.h>
TEST(parser, DISABLED_float_rounding_slow)
{
	static constexpr int num_threads = 8;
	std::vector<std::thread> threads;
	for(int32_t i = 0; i < num_threads; ++i)
	{
		threads.emplace_back([i]
		{
			ReusableStorage<std::string> storage;
			ParseState state("", storage);
			uint32_t failures = 0;
			union
			{
				int32_t as_int;
				float as_float;
			}
			current;
			current.as_int = i;
			do
			{
				char buffer[32];
				sprintf(buffer, "%1.8e", current.as_float);
				ParseResult<float> result = FloatParser::parse_float<float>(state.NewText({ buffer, buffer + strlen(buffer) }));
				if (result.GetResult().IsSuccess())
				{
					float my_read = result.GetResult().GetSuccess().result;
					if (my_read != current.as_float && !isnan(current.as_float))
					{
						printf("\n\nmismatch on %1.8e vs %1.8e\n\n", current.as_float, my_read);
						++failures;
					}
				}
				else if (strcmp(buffer, "nan") != 0 && strcmp(buffer, "-nan") != 0)
				{
					printf("couldn't read %s\n", buffer);
				}
				current.as_int += num_threads;
				if ((current.as_int / num_threads) % 10000000 == 0) printf("progress %d: %1.8e\n", current.as_int, current.as_float);
			}
			while(current.as_int != i);
			printf("num failures: %u\n", failures);
		});
	}
	for (std::thread & thread : threads) thread.join();
}

TEST(parser, parse_string)
{
	ReusableStorage<std::string> storage;
	ParseState state("", storage);
	{
		auto result = parse_string(state.NewText("\"foo\""));
		ASSERT_TRUE(result.GetResult().IsSuccess());
		ASSERT_EQ("foo", result.GetResult().GetSuccess().result.object);
	}
	{
		auto result = parse_string(state.NewText("\"hello, world\\n\""));
		ASSERT_TRUE(result.GetResult().IsSuccess());
		ASSERT_EQ("hello, world\n", result.GetResult().GetSuccess().result.object);
	}
	{
		auto result = parse_string(state.NewText("\"hello\\\", world\\n\""));
		ASSERT_TRUE(result.GetResult().IsSuccess());
		ASSERT_EQ("hello\", world\n", result.GetResult().GetSuccess().result.object);
	}
}

TEST(parser, parse_bool)
{
	ReusableStorage<std::string> storage;
	ParseState state("", storage);
	ASSERT_PARSE_RESULT(true, parse_bool(state.NewText("true")));
	ASSERT_PARSE_RESULT(false, parse_bool(state.NewText("false")));
	ASSERT_FALSE(parse_bool(state.NewText("foo")).GetResult().IsSuccess());
	// the reason why I accept this following input is that it's unlikely that
	// the error will propagate far from the boolean reading
	ASSERT_PARSE_RESULT(false, parse_bool(state.NewText("falsee")));
}

#endif
