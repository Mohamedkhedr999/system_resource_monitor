#include "core/process/process_monitor_impl.hpp"

#include <algorithm>
#include <chrono>
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
#include "core/process/process_snapshot.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "tests/mocks/mock_process_reader.hpp"

namespace pulse {
namespace {

using ProcessReadResult = std::expected<std::vector<ProcessSnapshot>, ProcessReadError>;

ProcessReadResult readResult(const std::vector<ProcessInfo>& processes) {
    std::vector<ProcessSnapshot> snapshots;
    for (const auto& info : processes) {
        snapshots.push_back({.info = info, .cpu_stats = {.pid = info.pid}});
    }
    return snapshots;
}

ProcessSnapshot snapshot(std::int32_t pid, std::uint64_t user_ticks, std::uint64_t system_ticks = 0,
                         std::uint64_t start_time = 100, std::uint64_t memory_kb = 10) {
    return {.info = {.pid = pid, .name = "worker", .memory_kb = memory_kb},
            .cpu_stats = {.pid = pid,
                          .user_ticks = user_ticks,
                          .system_ticks = system_ticks,
                          .start_time_ticks = start_time}};
}

class ProcessMonitorImplTest : public testing::Test {
   protected:
    testing::InSequence sequence_;
    ProcessMonitorImpl::Clock::time_point time_{};

    void expectSnapshots(std::vector<ProcessSnapshot> snapshots) {
        EXPECT_CALL(*reader_, read())
            .WillOnce(testing::Return(ProcessReadResult{std::move(snapshots)}));
    }

    std::unique_ptr<testing::StrictMock<MockProcessReader>> reader_ =
        std::make_unique<testing::StrictMock<MockProcessReader>>();
};

TEST(ProcessMonitorImplConstructorTest, RejectsNullReader) {
    EXPECT_THROW((void)ProcessMonitorImpl(nullptr, 100), std::invalid_argument);
}

TEST_F(ProcessMonitorImplTest, ReturnsEmptyListWhenReaderReturnsNoProcesses) {
    EXPECT_CALL(*reader_, read()).WillOnce(testing::Return(ProcessReadResult{}));
    ProcessMonitorImpl monitor(std::move(reader_), 100);

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
    EXPECT_CALL(*reader_, read()).WillOnce(testing::Return(readResult(processes)));
    ProcessMonitorImpl monitor(std::move(reader_), 100);

    const auto result = monitor.sample();

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(
        *result,
        testing::ElementsAre(
            testing::FieldsAre(2, "large", 'R', std::optional<std::uint64_t>(500), std::nullopt),
            testing::FieldsAre(9, "medium", 'D', std::optional<std::uint64_t>(100), std::nullopt),
            testing::FieldsAre(4, "small", 'S', std::optional<std::uint64_t>(10), std::nullopt)));
}

TEST_F(ProcessMonitorImplTest, RanksUnknownMemoryAfterKnownZeroAndMaximumMemory) {
    const std::vector<ProcessInfo> processes{
        {.pid = 1, .name = {}, .memory_kb = std::nullopt},
        {.pid = 2, .name = {}, .memory_kb = 0},
        {.pid = 3, .name = {}, .memory_kb = std::numeric_limits<std::uint64_t>::max()},
    };
    EXPECT_CALL(*reader_, read()).WillOnce(testing::Return(readResult(processes)));
    ProcessMonitorImpl monitor(std::move(reader_), 100);

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
    EXPECT_CALL(*reader_, read()).WillOnce(testing::Return(readResult(processes)));
    ProcessMonitorImpl monitor(std::move(reader_), 100);

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
    EXPECT_CALL(*reader_, read()).WillOnce(testing::Return(readResult(processes)));
    ProcessMonitorImpl monitor(std::move(reader_), 100);

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
    EXPECT_CALL(*reader_, read()).WillOnce(testing::Return(readResult(processes)));
    ProcessMonitorImpl monitor(std::move(reader_), 100, ProcessMonitorImpl::Clock::now, GetParam());

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

TEST_F(ProcessMonitorImplTest, RejectsZeroClockTickRate) {
    EXPECT_THROW((void)ProcessMonitorImpl(std::move(reader_), 0), std::invalid_argument);
}

TEST_F(ProcessMonitorImplTest, RejectsEmptyClockCallable) {
    EXPECT_THROW((void)ProcessMonitorImpl(std::move(reader_), 100, {}), std::invalid_argument);
}

TEST_F(ProcessMonitorImplTest, CalculatesCpuUsingElapsedTimeAndBothCounters) {
    auto first_snapshot = snapshot(1, 100, 50);
    first_snapshot.info.cpu_percentage = 999.0;
    expectSnapshots({first_snapshot});
    expectSnapshots({snapshot(1, 130, 70)});
    expectSnapshots({snapshot(1, 130, 70)});
    ProcessMonitorImpl monitor(std::move(reader_), 100, [this] { return time_; });

    const auto first = monitor.sample();
    ASSERT_TRUE(first.has_value());
    ASSERT_EQ(first->size(), 1U);
    EXPECT_FALSE(first->front().cpu_percentage.has_value());

    time_ += std::chrono::seconds{2};
    const auto second = monitor.sample();
    ASSERT_TRUE(second.has_value());
    ASSERT_EQ(second->size(), 1U);
    EXPECT_THAT(second->front().cpu_percentage, testing::Optional(testing::DoubleEq(25.0)));

    time_ += std::chrono::milliseconds{500};
    const auto third = monitor.sample();
    ASSERT_TRUE(third.has_value());
    ASSERT_EQ(third->size(), 1U);
    EXPECT_THAT(third->front().cpu_percentage, testing::Optional(testing::DoubleEq(0.0)));
}

TEST_F(ProcessMonitorImplTest, LeavesNewProcessesUnavailableAndAllowsMulticoreUsage) {
    expectSnapshots({snapshot(1, 100)});
    expectSnapshots({snapshot(1, 250, 50), snapshot(2, 80)});
    ProcessMonitorImpl monitor(std::move(reader_), 100, [this] { return time_; });
    ASSERT_TRUE(monitor.sample().has_value());

    time_ += std::chrono::seconds{1};
    const auto result = monitor.sample();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 2U);
    EXPECT_THAT((*result)[0].cpu_percentage, testing::Optional(testing::DoubleEq(200.0)));
    EXPECT_FALSE((*result)[1].cpu_percentage.has_value());
}

TEST_F(ProcessMonitorImplTest, DropsTerminatedProcessesAndDoesNotReuseTheirHistory) {
    expectSnapshots({snapshot(1, 100)});
    expectSnapshots({});
    expectSnapshots({snapshot(1, 200)});
    ProcessMonitorImpl monitor(std::move(reader_), 100, [this] { return time_; });
    ASSERT_TRUE(monitor.sample().has_value());

    time_ += std::chrono::seconds{1};
    const auto empty = monitor.sample();
    ASSERT_TRUE(empty.has_value());
    EXPECT_TRUE(empty->empty());

    time_ += std::chrono::seconds{1};
    const auto returned = monitor.sample();
    ASSERT_TRUE(returned.has_value());
    ASSERT_EQ(returned->size(), 1U);
    EXPECT_FALSE(returned->front().cpu_percentage.has_value());
}

TEST_F(ProcessMonitorImplTest, RebaselinesAfterPidReuseAndEachDecreasingCounter) {
    expectSnapshots({snapshot(1, 100, 100, 100)});
    expectSnapshots({snapshot(1, 100, 100, 200)});
    expectSnapshots({snapshot(1, 150, 100, 200)});
    expectSnapshots({snapshot(1, 149, 200, 200)});
    expectSnapshots({snapshot(1, 199, 200, 200)});
    expectSnapshots({snapshot(1, 299, 199, 200)});
    expectSnapshots({snapshot(1, 349, 199, 200)});
    ProcessMonitorImpl monitor(std::move(reader_), 100, [this] { return time_; });

    const std::vector<std::optional<double>> expected{
        std::nullopt, std::nullopt, 50.0, std::nullopt, 50.0, std::nullopt, 50.0};
    for (const auto cpu : expected) {
        SCOPED_TRACE(time_.time_since_epoch().count());
        const auto result = monitor.sample();
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(result->size(), 1U);
        EXPECT_EQ(result->front().cpu_percentage, cpu);
        time_ += std::chrono::seconds{1};
    }
}

TEST_F(ProcessMonitorImplTest, PreservesCountersAndTimestampAcrossReadFailure) {
    expectSnapshots({snapshot(1, 100)});
    EXPECT_CALL(*reader_, read())
        .WillOnce(testing::Return(
            ProcessReadResult{std::unexpected(ProcessReadError::DirectoryReadFailed)}));
    expectSnapshots({snapshot(1, 200)});
    ProcessMonitorImpl monitor(std::move(reader_), 100, [this] { return time_; });
    ASSERT_TRUE(monitor.sample().has_value());

    time_ += std::chrono::seconds{1};
    const auto failed = monitor.sample();
    ASSERT_FALSE(failed.has_value());
    EXPECT_EQ(failed.error(), ProcessReadError::DirectoryReadFailed);

    time_ += std::chrono::seconds{1};
    const auto recovered = monitor.sample();
    ASSERT_TRUE(recovered.has_value());
    ASSERT_EQ(recovered->size(), 1U);
    EXPECT_THAT(recovered->front().cpu_percentage, testing::Optional(testing::DoubleEq(50.0)));
}

TEST_F(ProcessMonitorImplTest, TracksProcessesOutsideTheDisplayedLimit) {
    expectSnapshots({snapshot(1, 100, 0, 100, 20), snapshot(2, 100, 0, 100, 10)});
    expectSnapshots({snapshot(1, 100, 0, 100, 20), snapshot(2, 150, 0, 100, 30)});
    ProcessMonitorImpl monitor(std::move(reader_), 100, [this] { return time_; }, 1);
    const auto first = monitor.sample();
    ASSERT_TRUE(first.has_value());
    ASSERT_EQ(first->size(), 1U);
    EXPECT_EQ(first->front().pid, 1);

    time_ += std::chrono::seconds{1};
    const auto second = monitor.sample();
    ASSERT_TRUE(second.has_value());
    ASSERT_EQ(second->size(), 1U);
    EXPECT_EQ(second->front().pid, 2);
    EXPECT_THAT(second->front().cpu_percentage, testing::Optional(testing::DoubleEq(50.0)));
}

TEST_F(ProcessMonitorImplTest, RebaselinesAfterNonpositiveElapsedTime) {
    expectSnapshots({snapshot(1, 100)});
    expectSnapshots({snapshot(1, 150)});
    expectSnapshots({snapshot(1, 200)});
    expectSnapshots({snapshot(1, 250)});
    ProcessMonitorImpl monitor(std::move(reader_), 100, [this] { return time_; });
    ASSERT_TRUE(monitor.sample().has_value());

    const auto same_time = monitor.sample();
    ASSERT_TRUE(same_time.has_value());
    ASSERT_EQ(same_time->size(), 1U);
    EXPECT_FALSE(same_time->front().cpu_percentage.has_value());

    time_ -= std::chrono::seconds{1};
    const auto earlier_time = monitor.sample();
    ASSERT_TRUE(earlier_time.has_value());
    ASSERT_EQ(earlier_time->size(), 1U);
    EXPECT_FALSE(earlier_time->front().cpu_percentage.has_value());

    time_ += std::chrono::seconds{1};
    const auto recovered = monitor.sample();
    ASSERT_TRUE(recovered.has_value());
    ASSERT_EQ(recovered->size(), 1U);
    EXPECT_THAT(recovered->front().cpu_percentage, testing::Optional(testing::DoubleEq(50.0)));
}

TEST_F(ProcessMonitorImplTest, PropagatesReaderErrorAndReadsFreshDataAfterFailure) {
    EXPECT_CALL(*reader_, read())
        .WillOnce(testing::Return(
            ProcessReadResult(std::unexpected(ProcessReadError::DirectoryReadFailed))))
        .WillOnce(testing::Return(
            readResult(std::vector<ProcessInfo>{{.pid = 7, .name = {}, .memory_kb = 100}})))
        .WillOnce(testing::Return(
            readResult(std::vector<ProcessInfo>{{.pid = 8, .name = {}, .memory_kb = 200}})));
    ProcessMonitorImpl monitor(std::move(reader_), 100);

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
