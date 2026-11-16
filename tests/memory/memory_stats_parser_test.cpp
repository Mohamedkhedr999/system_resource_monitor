#include "core/memory/memory_stats_parser.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include "gtest/gtest.h"

namespace pulse {
namespace {

TEST(MemoryStatsParserTest, MapsRequiredFieldsAndIgnoresUnrelatedLines) {
    const auto result = MemoryStatsParser::parse(
        "MemTotal: 16000 kB\nMemFree: 1000 kB\n"
        "Unrelated: ignored text\nMemAvailable: 10000 kB\n");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->total_kb, 16000U);
    EXPECT_EQ(result->available_kb, 10000U);
}

TEST(MemoryStatsParserTest, AcceptsReversedFieldsWhitespaceAndNoFinalNewline) {
    const auto result = MemoryStatsParser::parse(
        "\n \t\r\n\tMemAvailable:\t10000  kB \r\n  MemTotal:  16000\tkB  ");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->total_kb, 16000U);
    EXPECT_EQ(result->available_kb, 10000U);
}

TEST(MemoryStatsParserTest, AcceptsZeroAvailableAndAvailableEqualToTotal) {
    for (const std::string_view available : std::array{"0", "16000"}) {
        SCOPED_TRACE(available);
        const auto result = MemoryStatsParser::parse(
            "MemTotal: 16000 kB\nMemAvailable: " + std::string{available} + " kB\n");

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->total_kb, 16000U);
        EXPECT_EQ(result->available_kb, available == "0" ? 0U : 16000U);
    }
}

TEST(MemoryStatsParserTest, AcceptsMaximumUint64Values) {
    const auto result = MemoryStatsParser::parse(
        "MemTotal: 18446744073709551615 kB\nMemAvailable: 18446744073709551615 kB\n");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->total_kb, std::numeric_limits<std::uint64_t>::max());
    EXPECT_EQ(result->available_kb, std::numeric_limits<std::uint64_t>::max());
}

TEST(MemoryStatsParserTest, RejectsMissingRequiredFields) {
    for (const std::string_view content :
         std::array{"", " \t\n", "MemFree: 1000 kB\n", "MemTotal: 16000 kB\n",
                    "MemAvailable: 10000 kB\n", "MemTotal: 16000 kB\nmemavailable: 10000 kB\n",
                    "MemTotal 16000 kB\nMemAvailable: 10000 kB\n"}) {
        SCOPED_TRACE(content);
        const auto result = MemoryStatsParser::parse(content);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), MemoryParseError::MissingField);
    }
}

TEST(MemoryStatsParserTest, RejectsMalformedRequiredFields) {
    for (const std::string_view key : std::array{"MemTotal:", "MemAvailable:"}) {
        for (const std::string_view suffix :
             std::array{"", " 1", " kB", " 1 MB", " 1 kb", " 1 kB extra"}) {
            SCOPED_TRACE(key);
            SCOPED_TRACE(suffix);
            const auto result = MemoryStatsParser::parse(std::string{key} + std::string{suffix});

            ASSERT_FALSE(result.has_value());
            EXPECT_EQ(result.error(), MemoryParseError::InvalidFormat);
        }
    }
}

TEST(MemoryStatsParserTest, RejectsInvalidNumbersInEitherRequiredField) {
    for (const std::string_view key : std::array{"MemTotal:", "MemAvailable:"}) {
        for (const std::string_view value :
             std::array{"-1", "+1", "abc", "12abc", "1.5", "0x10", "18446744073709551616"}) {
            SCOPED_TRACE(key);
            SCOPED_TRACE(value);
            const auto result =
                MemoryStatsParser::parse(std::string{key} + " " + std::string{value} + " kB\n");

            ASSERT_FALSE(result.has_value());
            EXPECT_EQ(result.error(), MemoryParseError::InvalidValue);
        }
    }
}

TEST(MemoryStatsParserTest, RejectsDuplicateRequiredFieldsEvenAfterBothWereFound) {
    for (const std::string_view duplicate :
         std::array{"MemTotal: 16000 kB\n", "MemAvailable: 10000 kB\n"}) {
        SCOPED_TRACE(duplicate);
        const auto result = MemoryStatsParser::parse(
            "MemTotal: 16000 kB\nMemAvailable: 10000 kB\n" + std::string{duplicate});

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), MemoryParseError::InvalidFormat);
    }
}

TEST(MemoryStatsParserTest, RejectsZeroTotalAndAvailableGreaterThanTotal) {
    for (const std::string_view content : std::array{"MemTotal: 0 kB\nMemAvailable: 0 kB\n",
                                                     "MemTotal: 100 kB\nMemAvailable: 101 kB\n"}) {
        SCOPED_TRACE(content);
        const auto result = MemoryStatsParser::parse(content);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), MemoryParseError::InvalidValue);
    }
}

}  // namespace
}  // namespace pulse
