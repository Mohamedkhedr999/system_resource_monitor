#include "app/pulse_application.hpp"

#include <chrono>
#include <expected>
#include <future>
#include <memory>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "tests/mocks/mock_monitoring_service.hpp"

namespace pulse {
namespace {

using TickResult = std::expected<void, MonitoringError>;

class PulseApplicationTest : public testing::Test {
   protected:
    std::unique_ptr<testing::StrictMock<MockMonitoringService>> service_ =
        std::make_unique<testing::StrictMock<MockMonitoringService>>();
};

TEST_F(PulseApplicationTest, RejectsNullService) {
    EXPECT_THROW((void)PulseApplication(nullptr), std::invalid_argument);
}

TEST_F(PulseApplicationTest, RejectsZeroInterval) {
    EXPECT_THROW((void)PulseApplication(std::move(service_), std::chrono::milliseconds{0}),
                 std::invalid_argument);
}

TEST_F(PulseApplicationTest, RejectsNegativeInterval) {
    EXPECT_THROW((void)PulseApplication(std::move(service_), std::chrono::milliseconds{-1}),
                 std::invalid_argument);
}

TEST_F(PulseApplicationTest, AlreadyRequestedStopReturnsSuccessWithoutTicking) {
    EXPECT_CALL(*service_, tick()).Times(0);
    PulseApplication application(std::move(service_));
    std::stop_source stop_source;
    stop_source.request_stop();

    EXPECT_TRUE(application.run(stop_source.get_token()).has_value());
}

TEST_F(PulseApplicationTest, RepeatsSuccessfulTicksUntilStopped) {
    std::stop_source stop_source;
    EXPECT_CALL(*service_, tick())
        .WillOnce(testing::Return(TickResult{}))
        .WillOnce(testing::Return(TickResult{}))
        .WillOnce([&stop_source] {
            stop_source.request_stop();
            return TickResult{};
        });
    PulseApplication application(std::move(service_), std::chrono::milliseconds{1});

    EXPECT_TRUE(application.run(stop_source.get_token()).has_value());
}

TEST_F(PulseApplicationTest, StopRequestFinishesRunBeforeLongIntervalExpires) {
    constexpr auto kMonitoringInterval = std::chrono::seconds{30};
    constexpr auto kCompletionTimeout = std::chrono::seconds{5};
    std::promise<void> tick_started;
    auto tick_started_future = tick_started.get_future();
    std::promise<TickResult> finished;
    auto finished_future = finished.get_future();
    EXPECT_CALL(*service_, tick()).WillOnce([&tick_started] {
        tick_started.set_value();
        return TickResult{};
    });
    PulseApplication application(std::move(service_), kMonitoringInterval);
    std::jthread worker([&application, &finished](const std::stop_token& stop_token) {
        finished.set_value(application.run(stop_token));
    });

    // Synchronize with sampling instead of assuming the worker has started.
    ASSERT_EQ(tick_started_future.wait_for(kCompletionTimeout), std::future_status::ready);
    worker.request_stop();

    ASSERT_EQ(finished_future.wait_for(kCompletionTimeout), std::future_status::ready);
    EXPECT_TRUE(finished_future.get().has_value());
}

class PulseApplicationErrorTest : public PulseApplicationTest,
                                  public testing::WithParamInterface<MonitoringError> {};

TEST_P(PulseApplicationErrorTest, ReturnsTickErrorAndStopsSampling) {
    EXPECT_CALL(*service_, tick())
        .WillOnce(testing::Return(TickResult{std::unexpected(GetParam())}));
    PulseApplication application(std::move(service_));
    std::stop_source stop_source;

    const auto result = application.run(stop_source.get_token());

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), GetParam());
}

INSTANTIATE_TEST_SUITE_P(SamplingErrors, PulseApplicationErrorTest,
                         testing::Values(MonitoringError::CpuReadFailed,
                                         MonitoringError::CpuCalculationFailed,
                                         MonitoringError::MemoryReadFailed,
                                         MonitoringError::MemoryInvalidStats));

}  // namespace
}  // namespace pulse
