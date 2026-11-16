#include "core/memory/memory_monitor_impl.hpp"

#include <array>
#include <cstdint>
#include <expected>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

#include "core/memory/interfaces/memory_monitor.hpp"
#include "core/memory/interfaces/memory_reader.hpp"
#include "core/memory/memory_stats.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "tests/mocks/mock_memory_reader.hpp"

namespace pulse {
namespace {

using MemoryReadResult = std::expected<MemoryStats, MemoryReadError>;

class MemoryMonitorImplTest : public testing::Test {
   protected:
    std::unique_ptr<testing::StrictMock<MockMemoryReader>> reader_ =
        std::make_unique<testing::StrictMock<MockMemoryReader>>();
};

TEST(MemoryMonitorImplConstructorTest, RejectsNullReader) {
    EXPECT_THROW((void)MemoryMonitorImpl(nullptr), std::invalid_argument);
}

TEST_F(MemoryMonitorImplTest, FirstSampleReturnsUsedMemoryTotalAndFractionalPercentage) {
    EXPECT_CALL(*reader_, read())
        .WillOnce(testing::Return(MemoryReadResult(MemoryStats{.total_kb = 8, .available_kb = 5})));
    MemoryMonitorImpl monitor(std::move(reader_));

    const auto result = monitor.sample();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->used_kb, 3U);
    EXPECT_EQ(result->total_kb, 8U);
    EXPECT_DOUBLE_EQ(result->percentage, 37.5);
}

TEST_F(MemoryMonitorImplTest, ReturnsZeroUsageWhenAllMemoryIsAvailable) {
    EXPECT_CALL(*reader_, read())
        .WillOnce(
            testing::Return(MemoryReadResult(MemoryStats{.total_kb = 100, .available_kb = 100})));
    MemoryMonitorImpl monitor(std::move(reader_));

    const auto result = monitor.sample();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->used_kb, 0U);
    EXPECT_EQ(result->total_kb, 100U);
    EXPECT_DOUBLE_EQ(result->percentage, 0.0);
}

TEST_F(MemoryMonitorImplTest, HandlesMaximumCountersWithoutOverflow) {
    constexpr auto kMaximum = std::numeric_limits<std::uint64_t>::max();
    EXPECT_CALL(*reader_, read())
        .WillOnce(testing::Return(
            MemoryReadResult(MemoryStats{.total_kb = kMaximum, .available_kb = 0})));
    MemoryMonitorImpl monitor(std::move(reader_));

    const auto result = monitor.sample();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->used_kb, kMaximum);
    EXPECT_EQ(result->total_kb, kMaximum);
    EXPECT_DOUBLE_EQ(result->percentage, 100.0);
}

TEST_F(MemoryMonitorImplTest, MapsAllReaderErrorsToReadFailed) {
    EXPECT_CALL(*reader_, read())
        .WillOnce(
            testing::Return(MemoryReadResult(std::unexpected(MemoryReadError::FileReadFailed))))
        .WillOnce(testing::Return(MemoryReadResult(std::unexpected(MemoryReadError::InvalidData))));
    MemoryMonitorImpl monitor(std::move(reader_));

    for (const auto error :
         std::array{MemoryReadError::FileReadFailed, MemoryReadError::InvalidData}) {
        SCOPED_TRACE(static_cast<int>(error));
        const auto result = monitor.sample();

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), MemoryMonitorError::ReadFailed);
    }
}

TEST_F(MemoryMonitorImplTest, RejectsZeroTotalAndAvailableGreaterThanTotal) {
    constexpr std::array invalid_stats{
        MemoryStats{.total_kb = 0, .available_kb = 0},
        MemoryStats{.total_kb = 0, .available_kb = 1},
        MemoryStats{.total_kb = 100, .available_kb = 101},
    };
    auto& reader = *reader_;
    MemoryMonitorImpl monitor(std::move(reader_));

    for (const auto stats : invalid_stats) {
        SCOPED_TRACE(stats.total_kb);
        SCOPED_TRACE(stats.available_kb);
        EXPECT_CALL(reader, read()).WillOnce(testing::Return(MemoryReadResult(stats)));

        const auto result = monitor.sample();

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), MemoryMonitorError::InvalidStats);
    }
}

TEST_F(MemoryMonitorImplTest, UsesFreshStatsOnEverySample) {
    EXPECT_CALL(*reader_, read())
        .WillOnce(
            testing::Return(MemoryReadResult(MemoryStats{.total_kb = 100, .available_kb = 40})))
        .WillOnce(
            testing::Return(MemoryReadResult(MemoryStats{.total_kb = 200, .available_kb = 150})));
    MemoryMonitorImpl monitor(std::move(reader_));

    const auto first = monitor.sample();
    const auto second = monitor.sample();

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first->used_kb, 60U);
    EXPECT_EQ(first->total_kb, 100U);
    EXPECT_DOUBLE_EQ(first->percentage, 60.0);
    EXPECT_EQ(second->used_kb, 50U);
    EXPECT_EQ(second->total_kb, 200U);
    EXPECT_DOUBLE_EQ(second->percentage, 25.0);
}

TEST_F(MemoryMonitorImplTest, RecoversAfterReadFailureAndInvalidStats) {
    EXPECT_CALL(*reader_, read())
        .WillOnce(
            testing::Return(MemoryReadResult(std::unexpected(MemoryReadError::FileReadFailed))))
        .WillOnce(testing::Return(MemoryReadResult(MemoryStats{})))
        .WillOnce(
            testing::Return(MemoryReadResult(MemoryStats{.total_kb = 100, .available_kb = 25})));
    MemoryMonitorImpl monitor(std::move(reader_));

    const auto failed_read = monitor.sample();
    ASSERT_FALSE(failed_read.has_value());
    EXPECT_EQ(failed_read.error(), MemoryMonitorError::ReadFailed);
    const auto invalid_stats = monitor.sample();
    ASSERT_FALSE(invalid_stats.has_value());
    EXPECT_EQ(invalid_stats.error(), MemoryMonitorError::InvalidStats);

    const auto recovered = monitor.sample();

    ASSERT_TRUE(recovered.has_value());
    EXPECT_EQ(recovered->used_kb, 75U);
    EXPECT_EQ(recovered->total_kb, 100U);
    EXPECT_DOUBLE_EQ(recovered->percentage, 75.0);
}

}  // namespace
}  // namespace pulse
