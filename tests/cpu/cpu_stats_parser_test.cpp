#include "core/cpu/cpu_stats_parser.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <string>
#include <string_view>

#include "gtest/gtest.h"

namespace pulse {
namespace {

TEST(CpuStatsParserTest, MapsRequiredCountersToTheirFields) {
    const auto result = CpuStatsParser::parse("cpu 11 22 33 44 55 66 77 88");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->user, 11U);
    EXPECT_EQ(result->nice, 22U);
    EXPECT_EQ(result->system, 33U);
    EXPECT_EQ(result->idle, 44U);
    EXPECT_EQ(result->iowait, 55U);
    EXPECT_EQ(result->irq, 66U);
    EXPECT_EQ(result->softirq, 77U);
    EXPECT_EQ(result->steal, 88U);
}

TEST(CpuStatsParserTest, AcceptsAllZeroCounters) {
    const auto result = CpuStatsParser::parse("cpu 0 0 0 0 0 0 0 0");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->user, 0U);
    EXPECT_EQ(result->nice, 0U);
    EXPECT_EQ(result->system, 0U);
    EXPECT_EQ(result->idle, 0U);
    EXPECT_EQ(result->iowait, 0U);
    EXPECT_EQ(result->irq, 0U);
    EXPECT_EQ(result->softirq, 0U);
    EXPECT_EQ(result->steal, 0U);
}

TEST(CpuStatsParserTest, AcceptsWhitespaceBetweenAndAroundTokens) {
    const auto result = CpuStatsParser::parse(" \tcpu\t1  2\t3 4 5 6 7 8 \t\r\n");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->user, 1U);
    EXPECT_EQ(result->steal, 8U);
}

TEST(CpuStatsParserTest, AcceptsMaximumUint64Counter) {
    const auto result = CpuStatsParser::parse("cpu 18446744073709551615 0 0 0 0 0 0 0");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->user, std::numeric_limits<std::uint64_t>::max());
}

TEST(CpuStatsParserTest, AcceptsOneOrTwoGuestCountersWithoutAddingThemToUserOrNice) {
    for (const std::string_view line :
         {"cpu 100 50 3 4 5 6 7 8 10", "cpu 100 50 3 4 5 6 7 8 10 20"}) {
        SCOPED_TRACE(line);
        const auto result = CpuStatsParser::parse(line);

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->user, 100U);
        EXPECT_EQ(result->nice, 50U);
        EXPECT_EQ(result->steal, 8U);
    }
}

TEST(CpuStatsParserTest, RejectsMissingRequiredCountersAtEveryPosition) {
    std::string line = "cpu";
    for (std::size_t count = 0; count < 8; ++count) {
        SCOPED_TRACE(count);
        const auto result = CpuStatsParser::parse(line);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), CpuParseError::MissingFields);
        line += " 1";
    }
}

TEST(CpuStatsParserTest, RejectsInvalidNumbersAtEveryRequiredAndOptionalPosition) {
    constexpr std::array<std::string_view, 7> invalid_tokens{
        "abc", "12abc", "-1", "+1", "1.5", "0x10", "18446744073709551616"};

    for (const auto invalid_token : invalid_tokens) {
        for (std::size_t position = 0; position < 10; ++position) {
            SCOPED_TRACE(invalid_token);
            SCOPED_TRACE(position);
            std::string line = "cpu";
            for (std::size_t field = 0; field < 10; ++field) {
                line += ' ';
                line += field == position ? invalid_token : std::string_view{"1"};
            }
            const auto result = CpuStatsParser::parse(line);

            ASSERT_FALSE(result.has_value());
            EXPECT_EQ(result.error(), CpuParseError::InvalidNumber);
        }
    }
}

TEST(CpuStatsParserTest, AcceptsMaximumUint64OptionalCounters) {
    const auto result =
        CpuStatsParser::parse("cpu 1 2 3 4 5 6 7 8 18446744073709551615 18446744073709551615");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->user, 1U);
    EXPECT_EQ(result->nice, 2U);
}

TEST(CpuStatsParserTest, RejectsEmptyInputAndNonAggregateLabels) {
    for (const std::string_view line :
         {"", " \t", "cpu0 1 2 3 4 5 6 7 8", "CPU 1 2 3 4 5 6 7 8", "1 2 3 4 5 6 7 8"}) {
        SCOPED_TRACE(line);
        const auto result = CpuStatsParser::parse(line);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), CpuParseError::InvalidLabel);
    }
}

TEST(CpuStatsParserTest, RejectsAdditionalFieldsAfterTenCounters) {
    for (const std::string_view line :
         {"cpu 1 2 3 4 5 6 7 8 9 10 11", "cpu 1 2 3 4 5 6 7 8 9 10 extra"}) {
        SCOPED_TRACE(line);
        const auto result = CpuStatsParser::parse(line);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), CpuParseError::ExtraFields);
    }
}

}  // namespace
}  // namespace pulse
