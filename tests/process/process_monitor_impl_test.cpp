#include "core/process/process_monitor_impl.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "core/process/interfaces/process_reader.hpp"
#include "core/process/process_info.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "tests/mocks/mock_process_reader.hpp"

namespace pulse {
namespace {

using ProcessReadResult = std::expected<std::vector<ProcessInfo>, ProcessReadError>;

class ProcessMonitorImplTest : public testing::Test {
   protected:
    std::unique_ptr<testing::StrictMock<MockProcessReader>> reader_ =
        std::make_unique<testing::StrictMock<MockProcessReader>>();
};

TEST(ProcessMonitorImplConstructorTest, RejectsNullReader) {
    EXPECT_THROW((void)ProcessMonitorImpl(nullptr), std::invalid_argument);
}

TEST_F(ProcessMonitorImplTest, ReturnsEmptyListWhenReaderReturnsNoProcesses) {
    EXPECT_CALL(*reader_, read()).WillOnce(testing::Return(ProcessReadResult{}));
    ProcessMonitorImpl monitor(std::move(reader_));

    const auto result = monitor.sample();

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST_F(ProcessMonitorImplTest, SortsByMemoryAndPreservesProcessDetails) {
    const std::vector<ProcessInfo> processes{
        {.pid = 4, .name = "small", .state = 'S', .memory_kb = 10},
        {.pid = 2, .name = "large", .state = 'R', .memory_kb = 500},
        {.pid = 9, .name = "medium", .state = 'D', .memory_kb = 100},
    };
    EXPECT_CALL(*reader_, read()).WillOnce(testing::Return(ProcessReadResult(processes)));
    ProcessMonitorImpl monitor(std::move(reader_));

    const auto result = monitor.sample();

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(*result,
                testing::ElementsAre(
                    testing::FieldsAre(2, "large", 'R', std::optional<std::uint64_t>(500)),
                    testing::FieldsAre(9, "medium", 'D', std::optional<std::uint64_t>(100)),
                    testing::FieldsAre(4, "small", 'S', std::optional<std::uint64_t>(10))));
}

TEST_F(ProcessMonitorImplTest, RanksUnknownMemoryAfterKnownZeroAndMaximumMemory) {
    const std::vector<ProcessInfo> processes{
        {.pid = 1, .name = {}, .memory_kb = std::nullopt},
        {.pid = 2, .name = {}, .memory_kb = 0},
        {.pid = 3, .name = {}, .memory_kb = std::numeric_limits<std::uint64_t>::max()},
    };
    EXPECT_CALL(*reader_, read()).WillOnce(testing::Return(ProcessReadResult(processes)));
    ProcessMonitorImpl monitor(std::move(reader_));

    const auto result = monitor.sample();

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(*result, testing::ElementsAre(testing::Field(&ProcessInfo::pid, 3),
                                              testing::Field(&ProcessInfo::pid, 2),
                                              testing::Field(&ProcessInfo::pid, 1)));
}

TEST_F(ProcessMonitorImplTest, PreservesReaderOrderForEqualAndUnknownMemory) {
    const std::vector<ProcessInfo> processes{
        {.pid = 4, .name = {}, .memory_kb = std::nullopt},
        {.pid = 3, .name = {}, .memory_kb = 100},
        {.pid = 2, .name = {}, .memory_kb = 100},
        {.pid = 1, .name = {}, .memory_kb = std::nullopt},
    };
    EXPECT_CALL(*reader_, read()).WillOnce(testing::Return(ProcessReadResult(processes)));
    ProcessMonitorImpl monitor(std::move(reader_));

    const auto result = monitor.sample();

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(*result, testing::ElementsAre(testing::Field(&ProcessInfo::pid, 3),
                                              testing::Field(&ProcessInfo::pid, 2),
                                              testing::Field(&ProcessInfo::pid, 4),
                                              testing::Field(&ProcessInfo::pid, 1)));
}

TEST_F(ProcessMonitorImplTest, DefaultsToTenHighestMemoryProcesses) {
    std::vector<ProcessInfo> processes;
    for (std::int32_t pid = 1; pid <= 12; ++pid) {
        processes.push_back({.pid = pid, .name = {}, .memory_kb = static_cast<std::uint64_t>(pid)});
    }
    EXPECT_CALL(*reader_, read()).WillOnce(testing::Return(ProcessReadResult(processes)));
    ProcessMonitorImpl monitor(std::move(reader_));

    const auto result = monitor.sample();

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 10U);
    for (std::size_t index = 0; index < result->size(); ++index) {
        EXPECT_EQ((*result)[index].pid, 12 - static_cast<std::int32_t>(index));
    }
}

class ProcessMonitorLimitTest : public ProcessMonitorImplTest,
                                public testing::WithParamInterface<std::size_t> {};

TEST_P(ProcessMonitorLimitTest, ReturnsAtMostConfiguredLimitInDescendingOrder) {
    const std::vector<ProcessInfo> processes{
        {.pid = 1, .name = {}, .memory_kb = 10},
        {.pid = 2, .name = {}, .memory_kb = 20},
        {.pid = 3, .name = {}, .memory_kb = 30},
    };
    EXPECT_CALL(*reader_, read()).WillOnce(testing::Return(ProcessReadResult(processes)));
    ProcessMonitorImpl monitor(std::move(reader_), GetParam());

    const auto result = monitor.sample();

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), std::min(GetParam(), processes.size()));
    for (std::size_t index = 0; index < result->size(); ++index) {
        EXPECT_EQ((*result)[index].pid, 3 - static_cast<std::int32_t>(index));
    }
}

INSTANTIATE_TEST_SUITE_P(ConfiguredLimits, ProcessMonitorLimitTest,
                         testing::Values(std::size_t{0}, std::size_t{1}, std::size_t{3},
                                         std::size_t{20}, std::numeric_limits<std::size_t>::max()));

TEST_F(ProcessMonitorImplTest, PropagatesReaderErrorAndReadsFreshDataAfterFailure) {
    EXPECT_CALL(*reader_, read())
        .WillOnce(testing::Return(
            ProcessReadResult(std::unexpected(ProcessReadError::DirectoryReadFailed))))
        .WillOnce(testing::Return(
            ProcessReadResult(std::vector<ProcessInfo>{{.pid = 7, .name = {}, .memory_kb = 100}})))
        .WillOnce(testing::Return(
            ProcessReadResult(std::vector<ProcessInfo>{{.pid = 8, .name = {}, .memory_kb = 200}})));
    ProcessMonitorImpl monitor(std::move(reader_));

    const auto failed = monitor.sample();
    ASSERT_FALSE(failed.has_value());
    EXPECT_EQ(failed.error(), ProcessReadError::DirectoryReadFailed);

    const auto recovered = monitor.sample();
    ASSERT_TRUE(recovered.has_value());
    EXPECT_THAT(*recovered, testing::ElementsAre(testing::Field(&ProcessInfo::pid, 7)));

    const auto next = monitor.sample();
    ASSERT_TRUE(next.has_value());
    EXPECT_THAT(*next, testing::ElementsAre(testing::Field(&ProcessInfo::pid, 8)));
}

}  // namespace
}  // namespace pulse
