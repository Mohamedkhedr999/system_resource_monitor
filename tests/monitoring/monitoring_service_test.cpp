#include "core/monitoring/monitoring_service.hpp"

#include <expected>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

#include "core/cpu/interfaces/cpu_monitor.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "tests/mocks/mock_cpu_monitor.hpp"
#include "tests/mocks/mock_metrics_presenter.hpp"

namespace pulse {
namespace {

using SampleResult = std::expected<std::optional<double>, CpuMonitorError>;

class MonitoringServiceTest : public testing::Test {
   protected:
    std::unique_ptr<testing::StrictMock<MockCpuMonitor>> monitor_ =
        std::make_unique<testing::StrictMock<MockCpuMonitor>>();
    std::unique_ptr<testing::StrictMock<MockMetricsPresenter>> presenter_ =
        std::make_unique<testing::StrictMock<MockMetricsPresenter>>();
};

TEST_F(MonitoringServiceTest, RejectsNullMonitor) {
    EXPECT_THROW((void)MonitoringService(nullptr, std::move(presenter_)), std::invalid_argument);
}

TEST_F(MonitoringServiceTest, RejectsNullPresenter) {
    EXPECT_THROW((void)MonitoringService(std::move(monitor_), nullptr), std::invalid_argument);
}

TEST_F(MonitoringServiceTest, PresentsEachAvailableSampleIncludingZero) {
    constexpr double kUsage = 42.5;
    EXPECT_CALL(*monitor_, sample())
        .WillOnce(testing::Return(SampleResult(std::optional<double>{kUsage})))
        .WillOnce(testing::Return(SampleResult(std::optional<double>{0.0})));
    testing::InSequence sequence;
    EXPECT_CALL(*presenter_, presentCpuUsage(kUsage));
    EXPECT_CALL(*presenter_, presentCpuUsage(0.0));
    MonitoringService service(std::move(monitor_), std::move(presenter_));

    EXPECT_TRUE(service.tick().has_value());
    EXPECT_TRUE(service.tick().has_value());
}

TEST_F(MonitoringServiceTest, DoesNotPresentWarmUpSample) {
    EXPECT_CALL(*monitor_, sample()).WillOnce(testing::Return(SampleResult(std::nullopt)));
    EXPECT_CALL(*presenter_, presentCpuUsage(testing::_)).Times(0);
    MonitoringService service(std::move(monitor_), std::move(presenter_));

    EXPECT_TRUE(service.tick().has_value());
}

TEST_F(MonitoringServiceTest, ReturnsSamplingErrorsWithoutPresenting) {
    EXPECT_CALL(*monitor_, sample())
        .WillOnce(testing::Return(SampleResult(std::unexpected(CpuMonitorError::ReadFailed))))
        .WillOnce(
            testing::Return(SampleResult(std::unexpected(CpuMonitorError::CalculationFailed))));
    EXPECT_CALL(*presenter_, presentCpuUsage(testing::_)).Times(0);
    MonitoringService service(std::move(monitor_), std::move(presenter_));

    const auto read_failure = service.tick();
    ASSERT_FALSE(read_failure.has_value());
    EXPECT_EQ(read_failure.error(), CpuMonitorError::ReadFailed);

    const auto calculation_failure = service.tick();
    ASSERT_FALSE(calculation_failure.has_value());
    EXPECT_EQ(calculation_failure.error(), CpuMonitorError::CalculationFailed);
}

TEST_F(MonitoringServiceTest, ContinuesSamplingAfterFailureAndWarmUp) {
    constexpr double kUsage = 75.5;
    EXPECT_CALL(*monitor_, sample())
        .WillOnce(testing::Return(SampleResult(std::unexpected(CpuMonitorError::ReadFailed))))
        .WillOnce(testing::Return(SampleResult(std::nullopt)))
        .WillOnce(testing::Return(SampleResult(std::optional<double>{kUsage})));
    EXPECT_CALL(*presenter_, presentCpuUsage(kUsage)).Times(1);
    MonitoringService service(std::move(monitor_), std::move(presenter_));

    const auto failure = service.tick();
    ASSERT_FALSE(failure.has_value());
    EXPECT_EQ(failure.error(), CpuMonitorError::ReadFailed);
    EXPECT_TRUE(service.tick().has_value());
    EXPECT_TRUE(service.tick().has_value());
}

}  // namespace
}  // namespace pulse
