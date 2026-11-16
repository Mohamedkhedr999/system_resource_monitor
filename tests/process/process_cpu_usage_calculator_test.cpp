#include "core/process/process_cpu_usage_calculator.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <limits>

#include "core/process/process_cpu_stats.hpp"
#include "gtest/gtest.h"

namespace pulse {
namespace {

constexpr ProcessCpuStats kPrevious{
    .pid = 42, .user_ticks = 100, .system_ticks = 50, .start_time_ticks = 1000};

TEST(ProcessCpuUsageCalculatorTest, CalculatesUsageFromBothCounterDeltas) {
    auto current = kPrevious;
    current.user_ticks += 30;
    current.system_ticks += 20;

    const auto result =
        ProcessCpuUsageCalculator::calculate(kPrevious, current, std::chrono::seconds{2}, 100);

    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(*result, 25.0);
}

TEST(ProcessCpuUsageCalculatorTest, CountsUserAndSystemTimeIndependently) {
    for (const auto counter : {&ProcessCpuStats::user_ticks, &ProcessCpuStats::system_ticks}) {
        SCOPED_TRACE(counter == &ProcessCpuStats::user_ticks ? "user" : "system");
        auto current = kPrevious;
        current.*counter += 100;

        const auto result =
            ProcessCpuUsageCalculator::calculate(kPrevious, current, std::chrono::seconds{1}, 100);

        ASSERT_TRUE(result.has_value());
        EXPECT_DOUBLE_EQ(*result, 100.0);
    }
}

TEST(ProcessCpuUsageCalculatorTest, PreservesFractionalUsageWithSubsecondIntervals) {
    auto current = kPrevious;
    current.user_ticks += 1;

    const auto result = ProcessCpuUsageCalculator::calculate(kPrevious, current,
                                                             std::chrono::milliseconds{250}, 128);

    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(*result, 3.125);
}

TEST(ProcessCpuUsageCalculatorTest, ReturnsZeroForUnchangedCounters) {
    const auto result =
        ProcessCpuUsageCalculator::calculate(kPrevious, kPrevious, std::chrono::seconds{1}, 100);

    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(*result, 0.0);
}

TEST(ProcessCpuUsageCalculatorTest, AllowsUsageAboveOneHundredPercent) {
    auto current = kPrevious;
    current.user_ticks += 150;
    current.system_ticks += 50;

    const auto result =
        ProcessCpuUsageCalculator::calculate(kPrevious, current, std::chrono::seconds{1}, 100);

    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(*result, 200.0);
}

TEST(ProcessCpuUsageCalculatorTest, RejectsDifferentProcessIds) {
    auto current = kPrevious;
    ++current.pid;

    const auto result =
        ProcessCpuUsageCalculator::calculate(kPrevious, current, std::chrono::seconds{1}, 100);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProcessCpuCalculationError::ProcessChanged);
}

TEST(ProcessCpuUsageCalculatorTest, RejectsReusedProcessIdWithDifferentStartTime) {
    auto current = kPrevious;
    ++current.start_time_ticks;

    const auto result =
        ProcessCpuUsageCalculator::calculate(kPrevious, current, std::chrono::seconds{1}, 100);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProcessCpuCalculationError::ProcessChanged);
}

TEST(ProcessCpuUsageCalculatorTest, RejectsEachDecreasingCounterEvenWhenTheOtherIncreases) {
    const std::array counters{&ProcessCpuStats::user_ticks, &ProcessCpuStats::system_ticks};
    for (const auto counter : counters) {
        SCOPED_TRACE(counter == &ProcessCpuStats::user_ticks ? "user" : "system");
        auto current = kPrevious;
        current.user_ticks += 100;
        current.system_ticks += 100;
        current.*counter = kPrevious.*counter - 1;

        const auto result =
            ProcessCpuUsageCalculator::calculate(kPrevious, current, std::chrono::seconds{1}, 100);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), ProcessCpuCalculationError::InvalidSampleOrder);
    }
}

TEST(ProcessCpuUsageCalculatorTest, RejectsZeroAndNegativeElapsedTime) {
    for (const auto elapsed : {std::chrono::seconds{0}, std::chrono::seconds{-1}}) {
        SCOPED_TRACE(elapsed.count());
        const auto result =
            ProcessCpuUsageCalculator::calculate(kPrevious, kPrevious, elapsed, 100);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), ProcessCpuCalculationError::InvalidElapsedTime);
    }
}

TEST(ProcessCpuUsageCalculatorTest, RejectsZeroClockTicksPerSecond) {
    const auto result =
        ProcessCpuUsageCalculator::calculate(kPrevious, kPrevious, std::chrono::seconds{1}, 0);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProcessCpuCalculationError::InvalidClockTicks);
}

TEST(ProcessCpuUsageCalculatorTest, HandlesLargeAbsoluteCountersWithSmallDeltas) {
    constexpr auto max = std::numeric_limits<std::uint64_t>::max();
    auto previous = kPrevious;
    previous.user_ticks = max - 10;
    previous.system_ticks = max - 10;
    auto current = previous;
    current.user_ticks += 5;
    current.system_ticks += 5;

    const auto result =
        ProcessCpuUsageCalculator::calculate(previous, current, std::chrono::seconds{1}, 100);

    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(*result, 10.0);
}

TEST(ProcessCpuUsageCalculatorTest, HandlesCombinedDeltasExceedingUint64Maximum) {
    constexpr auto max = std::numeric_limits<std::uint64_t>::max();
    auto previous = kPrevious;
    previous.user_ticks = 0;
    previous.system_ticks = 0;
    auto current = previous;
    current.user_ticks = max;
    current.system_ticks = max;

    const auto result =
        ProcessCpuUsageCalculator::calculate(previous, current, std::chrono::seconds{1}, max);

    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(*result, 200.0);
}

}  // namespace
}  // namespace pulse
