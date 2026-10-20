#include "core/cpu/cpu_monitor.hpp"

#include <expected>
#include <initializer_list>
#include <limits>
#include <memory>
#include <stdexcept>

#include "core/cpu/cpu_stats.hpp"
#include "core/cpu/interfaces/cpu_monitor.hpp"
#include "core/cpu/interfaces/cpu_reader.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "tests/mocks/mock_cpu_reader.hpp"

namespace pulse {
namespace {

using CpuReadResult = std::expected<CpuStats, CpuReadError>;

class CpuMonitorTest : public testing::Test {
   protected:
    std::shared_ptr<testing::StrictMock<MockCpuReader>> reader_ =
        std::make_shared<testing::StrictMock<MockCpuReader>>();
};

TEST(CpuMonitorConstructorTest, RejectsNullReader) {
    EXPECT_THROW((void)CpuMonitor(nullptr), std::invalid_argument);
}

TEST_F(CpuMonitorTest, FirstSuccessfulSampleReturnsNoUsage) {
    EXPECT_CALL(*reader_, read()).WillOnce(testing::Return(CpuReadResult(CpuStats{})));
    CpuMonitor monitor(reader_);

    const auto result = monitor.sample();

    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->has_value());
}

TEST_F(CpuMonitorTest, CalculatesUsageUsingConsecutiveSamples) {
    EXPECT_CALL(*reader_, read())
        .WillOnce(testing::Return(CpuReadResult(CpuStats{.user = 100, .idle = 100})))
        .WillOnce(testing::Return(CpuReadResult(CpuStats{.user = 125, .idle = 175})))
        .WillOnce(testing::Return(CpuReadResult(CpuStats{.user = 200, .idle = 200})));
    CpuMonitor monitor(reader_);
    ASSERT_TRUE(monitor.sample().has_value());

    const auto second = monitor.sample();
    const auto third = monitor.sample();

    ASSERT_TRUE(second.has_value());
    ASSERT_TRUE(second->has_value());
    EXPECT_DOUBLE_EQ(**second, 25.0);
    ASSERT_TRUE(third.has_value());
    ASSERT_TRUE(third->has_value());
    EXPECT_DOUBLE_EQ(**third, 75.0);
}

TEST_F(CpuMonitorTest, MapsAllReaderErrorsToReadFailed) {
    CpuMonitor monitor(reader_);
    for (const auto error :
         {CpuReadError::FileOpenFailed, CpuReadError::FileReadFailed, CpuReadError::InvalidData}) {
        SCOPED_TRACE(static_cast<int>(error));
        EXPECT_CALL(*reader_, read())
            .WillOnce(testing::Return(CpuReadResult(std::unexpected(error))));

        const auto result = monitor.sample();

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), CpuMonitorError::ReadFailed);
    }
}

TEST_F(CpuMonitorTest, FailedInitialReadDoesNotEstablishBaseline) {
    EXPECT_CALL(*reader_, read())
        .WillOnce(testing::Return(CpuReadResult(std::unexpected(CpuReadError::FileReadFailed))))
        .WillOnce(testing::Return(CpuReadResult(CpuStats{.user = 10, .idle = 10})));
    CpuMonitor monitor(reader_);
    ASSERT_FALSE(monitor.sample().has_value());

    const auto result = monitor.sample();

    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->has_value());
}

TEST_F(CpuMonitorTest, ReadFailurePreservesLastSuccessfulBaseline) {
    EXPECT_CALL(*reader_, read())
        .WillOnce(testing::Return(CpuReadResult(CpuStats{.user = 100, .idle = 100})))
        .WillOnce(testing::Return(CpuReadResult(std::unexpected(CpuReadError::FileReadFailed))))
        .WillOnce(testing::Return(CpuReadResult(CpuStats{.user = 125, .idle = 175})));
    CpuMonitor monitor(reader_);
    ASSERT_TRUE(monitor.sample().has_value());
    const auto failed = monitor.sample();
    ASSERT_FALSE(failed.has_value());
    EXPECT_EQ(failed.error(), CpuMonitorError::ReadFailed);

    const auto recovered = monitor.sample();

    ASSERT_TRUE(recovered.has_value());
    ASSERT_TRUE(recovered->has_value());
    EXPECT_DOUBLE_EQ(**recovered, 25.0);
}

TEST_F(CpuMonitorTest, DecreasedCountersReturnCalculationFailedAndReplaceBaseline) {
    EXPECT_CALL(*reader_, read())
        .WillOnce(testing::Return(CpuReadResult(CpuStats{.user = 100, .idle = 100})))
        .WillOnce(testing::Return(CpuReadResult(CpuStats{.user = 10, .idle = 10})))
        .WillOnce(testing::Return(CpuReadResult(CpuStats{.user = 35, .idle = 85})));
    CpuMonitor monitor(reader_);
    ASSERT_TRUE(monitor.sample().has_value());

    const auto failed = monitor.sample();

    ASSERT_FALSE(failed.has_value());
    EXPECT_EQ(failed.error(), CpuMonitorError::CalculationFailed);

    const auto recovered = monitor.sample();

    ASSERT_TRUE(recovered.has_value());
    ASSERT_TRUE(recovered->has_value());
    EXPECT_DOUBLE_EQ(**recovered, 25.0);
}

TEST_F(CpuMonitorTest, IdenticalSamplesReturnCalculationFailed) {
    const CpuStats stats{.user = 10, .idle = 10};
    EXPECT_CALL(*reader_, read()).Times(2).WillRepeatedly(testing::Return(CpuReadResult(stats)));
    CpuMonitor monitor(reader_);
    ASSERT_TRUE(monitor.sample().has_value());

    const auto result = monitor.sample();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CpuMonitorError::CalculationFailed);
}

TEST_F(CpuMonitorTest, CounterOverflowReturnsCalculationFailed) {
    const CpuStats overflowing{.user = std::numeric_limits<std::uint64_t>::max(), .idle = 1};
    EXPECT_CALL(*reader_, read())
        .WillOnce(testing::Return(CpuReadResult(CpuStats{})))
        .WillOnce(testing::Return(CpuReadResult(overflowing)));
    CpuMonitor monitor(reader_);
    ASSERT_TRUE(monitor.sample().has_value());

    const auto result = monitor.sample();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CpuMonitorError::CalculationFailed);
}

TEST_F(CpuMonitorTest, ZeroUsageIsReturnedAsAValue) {
    EXPECT_CALL(*reader_, read())
        .WillOnce(testing::Return(CpuReadResult(CpuStats{})))
        .WillOnce(testing::Return(CpuReadResult(CpuStats{.idle = 10, .iowait = 10})));
    CpuMonitor monitor(reader_);
    ASSERT_TRUE(monitor.sample().has_value());

    const auto result = monitor.sample();

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_DOUBLE_EQ(**result, 0.0);
}

}  // namespace
}  // namespace pulse
