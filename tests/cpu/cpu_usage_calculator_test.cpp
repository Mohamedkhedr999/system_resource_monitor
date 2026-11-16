#include "core/cpu/cpu_usage_calculator.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "gtest/gtest.h"

namespace pulse {
namespace {

constexpr std::array kCounters{
    &CpuStats::user,   &CpuStats::nice, &CpuStats::system,  &CpuStats::idle,
    &CpuStats::iowait, &CpuStats::irq,  &CpuStats::softirq, &CpuStats::steal,
};
constexpr auto kMaxCounter = std::numeric_limits<std::uint64_t>::max();

TEST(CpuUsageCalculatorTest, CalculatesUsageFromDifferencesBetweenNonzeroSamples) {
    const CpuStats previous{10, 20, 30, 40, 50, 60, 70, 80};
    const CpuStats current{20, 30, 40, 50, 60, 70, 80, 90};

    const auto result = CpuUsageCalculator::calculate(previous, current);

    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(*result, 75.0);
}

TEST(CpuUsageCalculatorTest, PreservesFractionalPercentages) {
    const auto result = CpuUsageCalculator::calculate({}, CpuStats{.user = 1, .idle = 2});

    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(*result, 100.0 / 3.0);
}

TEST(CpuUsageCalculatorTest, CountsEachCounterAndTreatsIdleAndIowaitAsIdle) {
    for (const auto counter : kCounters) {
        CpuStats current{};
        current.*counter = 10;
        SCOPED_TRACE(counter == &CpuStats::idle || counter == &CpuStats::iowait ? "idle" : "busy");

        const auto result = CpuUsageCalculator::calculate({}, current);

        ASSERT_TRUE(result.has_value());
        const double expected =
            counter == &CpuStats::idle || counter == &CpuStats::iowait ? 0.0 : 100.0;
        EXPECT_DOUBLE_EQ(*result, expected);
    }
}

TEST(CpuUsageCalculatorTest, CombinesIdleAndIowaitDeltas) {
    const auto result =
        CpuUsageCalculator::calculate({}, CpuStats{.user = 30, .idle = 50, .iowait = 20});

    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(*result, 30.0);
}

TEST(CpuUsageCalculatorTest, RejectsIdenticalZeroAndNonzeroSamples) {
    for (const auto sample : {CpuStats{}, CpuStats{10, 20, 30, 40, 50, 60, 70, 80}}) {
        const auto result = CpuUsageCalculator::calculate(sample, sample);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), CpuCalculationError::ZeroElapsedTime);
    }
}

TEST(CpuUsageCalculatorTest, RejectsEachDecreasedCounterEvenWhenOthersIncrease) {
    const CpuStats previous{10, 10, 10, 10, 10, 10, 10, 10};
    for (std::size_t index = 0; index < kCounters.size(); ++index) {
        SCOPED_TRACE(index);
        CpuStats current{20, 20, 20, 20, 20, 20, 20, 20};
        current.*kCounters[index] = 9;

        const auto result = CpuUsageCalculator::calculate(previous, current);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), CpuCalculationError::InvalidSampleOrder);
    }
}

TEST(CpuUsageCalculatorTest, RejectsOverflowWhenCombiningCounterDeltas) {
    for (std::size_t index = 0; index < kCounters.size(); ++index) {
        SCOPED_TRACE(index);
        CpuStats current{};
        current.*kCounters[index] = kMaxCounter;
        current.*kCounters[(index + 1) % kCounters.size()] = 1;

        const auto result = CpuUsageCalculator::calculate({}, current);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), CpuCalculationError::CounterOverflow);
    }
}

TEST(CpuUsageCalculatorTest, AcceptsTotalDeltaExactlyAtUint64Maximum) {
    const CpuStats current{.user = kMaxCounter / 2, .idle = kMaxCounter - kMaxCounter / 2};

    const auto result = CpuUsageCalculator::calculate({}, current);

    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(*result, 50.0);
}

TEST(CpuUsageCalculatorTest, AcceptsLargeAbsoluteCountersWhenDeltaSumFits) {
    CpuStats previous{};
    CpuStats current{};
    for (const auto counter : kCounters) {
        previous.*counter = kMaxCounter - 10;
        current.*counter = kMaxCounter - 5;
    }

    const auto result = CpuUsageCalculator::calculate(previous, current);

    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(*result, 75.0);
}

TEST(CpuUsageCalculatorTest, RejectsCounterWraparoundAsInvalidSampleOrder) {
    const auto result =
        CpuUsageCalculator::calculate(CpuStats{.user = kMaxCounter}, CpuStats{.user = 0});

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CpuCalculationError::InvalidSampleOrder);
}

}  // namespace
}  // namespace pulse
