#include "core/process/process_stats_parser.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include "gtest/gtest.h"

namespace pulse {
namespace {

constexpr std::string_view valid_content =
    "Name:\tmy worker\nState:\tS (sleeping)\nPid:\t123\nVmRSS:\t4096 kB\n";

TEST(ProcessStatsParserTest, MapsRequiredFieldsAndIgnoresUnrelatedLines) {
    const auto result = ProcessStatsParser::parse(std::string{valid_content} +
                                                  "PPid: 42\nVmSize: invalid ignored value\n");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "my worker");
    EXPECT_EQ(result->state, 'S');
    EXPECT_EQ(result->pid, 123);
    EXPECT_EQ(result->memory_kb, 4096U);
}

TEST(ProcessStatsParserTest, AcceptsReorderedFieldsWhitespaceAndNoFinalNewline) {
    const auto result = ProcessStatsParser::parse(
        "\n \t\r\n  VmRSS:\t0 kB \r\nPid: 1\nState: R\n\tName:  worker name  ");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "worker name");
    EXPECT_EQ(result->state, 'R');
    EXPECT_EQ(result->pid, 1);
    EXPECT_EQ(result->memory_kb, 0U);
}

TEST(ProcessStatsParserTest, AcceptsMaximumNumericValues) {
    const auto result = ProcessStatsParser::parse(
        "Name: worker\nState: R\nPid: 2147483647\nVmRSS: 18446744073709551615 kB\n");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->pid, std::numeric_limits<std::int32_t>::max());
    EXPECT_EQ(result->memory_kb, std::numeric_limits<std::uint64_t>::max());
}

TEST(ProcessStatsParserTest, AcceptsSupportedStates) {
    for (const char state : std::string_view{"RSDZTtXI"}) {
        SCOPED_TRACE(state);
        const auto result = ProcessStatsParser::parse("Name: worker\nPid: 1\nVmRSS: 0 kB\nState: " +
                                                      std::string(1, state));

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->state, state);
    }
}

TEST(ProcessStatsParserTest, RejectsEmptyInput) {
    for (const std::string_view content : std::array{"", " \t\n", "Unrelated: value\n"}) {
        SCOPED_TRACE(content);
        const auto result = ProcessStatsParser::parse(content);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), ProcessParseError::MissingRequiredField);
    }
}

TEST(ProcessStatsParserTest, RejectsEachMissingRequiredField) {
    for (const std::string_view field : std::array{"Name:", "State:", "Pid:"}) {
        SCOPED_TRACE(field);
        std::string content{valid_content};
        const auto start = content.find(field);
        content.erase(start, content.find('\n', start) - start + 1);
        const auto result = ProcessStatsParser::parse(content);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), ProcessParseError::MissingRequiredField);
    }
}

TEST(ProcessStatsParserTest, ReturnsEmptyMemoryWhenVmRssIsAbsent) {
    const auto result = ProcessStatsParser::parse("Name: worker\nState: S (sleeping)\nPid: 123\n");

    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->memory_kb.has_value());
}

TEST(ProcessStatsParserTest, RejectsMalformedVmRssInOtherwiseValidRecord) {
    const auto result =
        ProcessStatsParser::parse("Name: worker\nState: R\nPid: 123\nVmRSS: invalid kB\n");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProcessParseError::InvalidField);
}

TEST(ProcessStatsParserTest, RejectsDuplicateZeroMemory) {
    const auto result =
        ProcessStatsParser::parse("Name: worker\nState: R\nPid: 123\nVmRSS: 0 kB\nVmRSS: 1 kB\n");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProcessParseError::DuplicateField);
}

TEST(ProcessStatsParserTest, RejectsDuplicateFieldsAfterCompleteRecord) {
    for (const std::string_view duplicate :
         std::array{"Name: other\n", "State: R\n", "Pid: 456\n", "VmRSS: 0 kB\n"}) {
        SCOPED_TRACE(duplicate);
        const auto result =
            ProcessStatsParser::parse(std::string{valid_content} + std::string{duplicate});

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), ProcessParseError::DuplicateField);
    }
}

TEST(ProcessStatsParserTest, RejectsEmptyNamesAndInvalidStates) {
    for (const std::string_view content :
         std::array{"Name:", "Name: \t\r", "State: Q", "State: Sleeping", "State: RS"}) {
        SCOPED_TRACE(content);
        const auto result = ProcessStatsParser::parse(content);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), ProcessParseError::InvalidField);
    }
}

TEST(ProcessStatsParserTest, RejectsMalformedRequiredFields) {
    for (const std::string_view content :
         std::array{"State:", "State: S sleeping", "State: S (sleeping", "State: S ()",
                    "State: S (sleeping) extra", "Pid:", "Pid: 1 extra", "VmRSS:", "VmRSS: 1",
                    "VmRSS: 1 MB", "VmRSS: 1 kb", "VmRSS: 1 kB extra"}) {
        SCOPED_TRACE(content);
        const auto result = ProcessStatsParser::parse(content);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), ProcessParseError::InvalidField);
    }
}

TEST(ProcessStatsParserTest, RejectsInvalidNumericValues) {
    for (const std::string_view value :
         std::array{"-1", "+1", "abc", "12abc", "1.5", "0x10", "18446744073709551616"}) {
        for (const std::string_view field : std::array{"Pid:", "VmRSS:"}) {
            SCOPED_TRACE(value);
            SCOPED_TRACE(field);
            const auto result = ProcessStatsParser::parse(
                std::string{field} + " " + std::string{value} + (field == "VmRSS:" ? " kB" : ""));

            ASSERT_FALSE(result.has_value());
            EXPECT_EQ(result.error(), ProcessParseError::InvalidField);
        }
    }
}

TEST(ProcessStatsParserTest, RejectsZeroAndOutOfRangePids) {
    for (const std::string_view content : std::array{"Pid: 0", "Pid: 2147483648"}) {
        SCOPED_TRACE(content);
        const auto result = ProcessStatsParser::parse(content);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), ProcessParseError::InvalidField);
    }
}

}  // namespace
}  // namespace pulse
