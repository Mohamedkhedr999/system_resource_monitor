#include "core/process/process_cpu_stats_parser.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include "gtest/gtest.h"

namespace pulse {
namespace {

std::string record(std::string_view pid = "123", std::string_view name = "worker",
                   std::string_view user = "140", std::string_view system = "150",
                   std::string_view start = "220", int last_field = 24) {
    std::string content = std::string{pid} + " (" + std::string{name} + ") S";
    for (int field = 4; field <= last_field; ++field) {
        content += " ";
        if (field == 14) {
            content += user;
        } else if (field == 15) {
            content += system;
        } else if (field == 22) {
            content += start;
        } else {
            content += std::to_string(field);
        }
    }
    return content;
}

TEST(ProcessCpuStatsParserTest, MapsFieldsAndAllowsLaterFields) {
    const auto result = ProcessCpuStatsParser::parse(record());

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->pid, 123);
    EXPECT_EQ(result->user_ticks, 140U);
    EXPECT_EQ(result->system_ticks, 150U);
    EXPECT_EQ(result->start_time_ticks, 220U);
}

TEST(ProcessCpuStatsParserTest, HandlesSpacesAndParenthesesInNames) {
    for (const std::string_view name :
         std::array{"my worker", "a (nested) worker)", ")(", "", "worker\nname"}) {
        SCOPED_TRACE(name);
        const auto result = ProcessCpuStatsParser::parse(record("1", name));

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->user_ticks, 140U);
        EXPECT_EQ(result->start_time_ticks, 220U);
    }
}

TEST(ProcessCpuStatsParserTest, AcceptsWhitespaceAndExactlyTwentyTwoFields) {
    auto content = record("123", "worker", "140", "150", "220", 22);
    for (char& character : content) {
        if (character == ' ') {
            character = '\t';
        }
    }
    const auto result = ProcessCpuStatsParser::parse(" \r\n" + content + "\n");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->pid, 123);
    EXPECT_EQ(result->system_ticks, 150U);
}

TEST(ProcessCpuStatsParserTest, AcceptsZeroAndMaximumCounters) {
    for (const std::string_view value : std::array{"0", "18446744073709551615"}) {
        const auto result =
            ProcessCpuStatsParser::parse(record("2147483647", "worker", value, value, value));

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->pid, std::numeric_limits<std::int32_t>::max());
        const auto expected = value == "0" ? 0 : std::numeric_limits<std::uint64_t>::max();
        EXPECT_EQ(result->user_ticks, expected);
        EXPECT_EQ(result->system_ticks, expected);
        EXPECT_EQ(result->start_time_ticks, expected);
    }
}

TEST(ProcessCpuStatsParserTest, RejectsMissingPidAndEmptyInput) {
    for (const std::string_view content : std::array{"", " \t\n", "(worker) S"}) {
        const auto result = ProcessCpuStatsParser::parse(content);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), ProcessCpuParseError::MissingField);
    }
}

TEST(ProcessCpuStatsParserTest, RejectsTruncatedRecords) {
    for (int last_field = 3; last_field < 22; ++last_field) {
        SCOPED_TRACE(last_field);
        const auto result =
            ProcessCpuStatsParser::parse(record("123", "worker", "140", "150", "220", last_field));

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), ProcessCpuParseError::MissingField);
    }
    const auto result = ProcessCpuStatsParser::parse("123 (worker)");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProcessCpuParseError::MissingField);
}

TEST(ProcessCpuStatsParserTest, RejectsInvalidPids) {
    for (const std::string_view pid :
         std::array{"0", "-1", "+1", "abc", "12abc", "1 2", "2147483648"}) {
        SCOPED_TRACE(pid);
        const auto result = ProcessCpuStatsParser::parse(record(pid));

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), ProcessCpuParseError::InvalidField);
    }
}

TEST(ProcessCpuStatsParserTest, RejectsMalformedNameDelimiters) {
    for (const std::string_view content :
         std::array{"123 worker S", "123 (worker S", "123 )worker( S", "123(worker) S",
                    "123 (worker)S 4"}) {
        SCOPED_TRACE(content);
        const auto result = ProcessCpuStatsParser::parse(content);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), ProcessCpuParseError::InvalidField);
    }
}

TEST(ProcessCpuStatsParserTest, RejectsInvalidCountersInEachSelectedField) {
    for (const std::string_view value :
         std::array{"-1", "+1", "abc", "12abc", "1.5", "0x10", "18446744073709551616"}) {
        const std::array records{record("123", "worker", value),
                                 record("123", "worker", "140", value),
                                 record("123", "worker", "140", "150", value)};
        for (const auto& content : records) {
            SCOPED_TRACE(content);
            const auto result = ProcessCpuStatsParser::parse(content);

            ASSERT_FALSE(result.has_value());
            EXPECT_EQ(result.error(), ProcessCpuParseError::InvalidField);
        }
    }
}

}  // namespace
}  // namespace pulse
