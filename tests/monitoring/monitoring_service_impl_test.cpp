#include "core/monitoring/monitoring_service_impl.hpp"

#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "core/cpu/interfaces/cpu_monitor.hpp"
#include "core/memory/interfaces/memory_monitor.hpp"
#include "core/memory/memory_usage.hpp"
#include "core/monitoring/interfaces/monitoring_service.hpp"
#include "core/process/interfaces/process_reader.hpp"
#include "core/process/process_info.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "tests/mocks/mock_cpu_monitor.hpp"
#include "tests/mocks/mock_memory_monitor.hpp"
#include "tests/mocks/mock_metrics_presenter.hpp"
#include "tests/mocks/mock_process_monitor.hpp"

namespace pulse {
namespace {

using SampleResult = std::expected<std::optional<double>, CpuMonitorError>;
using MemorySampleResult = std::expected<MemoryUsage, MemoryMonitorError>;
using ProcessSampleResult = std::expected<std::vector<ProcessInfo>, ProcessReadError>;

class MonitoringServiceImplTest : public testing::Test {
   protected:
    std::unique_ptr<testing::StrictMock<MockMemoryMonitor>> memory_monitor_ =
        std::make_unique<testing::StrictMock<MockMemoryMonitor>>();

    std::unique_ptr<testing::StrictMock<MockProcessMonitor>> process_monitor_ =
        std::make_unique<testing::StrictMock<MockProcessMonitor>>();

    void expectProcessSamples(int count) {
        EXPECT_CALL(*process_monitor_, sample())
            .Times(count)
            .WillRepeatedly(testing::Return(ProcessSampleResult{}));
        EXPECT_CALL(*presenter_, presentProcesses(testing::IsEmpty())).Times(count);
    }

    void expectMemorySamples(int count) {
        EXPECT_CALL(*memory_monitor_, sample())
            .Times(count)
            .WillRepeatedly(testing::Return(MemorySampleResult{
                MemoryUsage{.used_kb = 250, .total_kb = 1000, .percentage = 25.0}}));
        EXPECT_CALL(*presenter_, presentMemoryUsage(testing::AllOf(
                                     testing::Field(&MemoryUsage::used_kb, 250),
                                     testing::Field(&MemoryUsage::total_kb, 1000),
                                     testing::Field(&MemoryUsage::percentage, 25.0))))
            .Times(count);
    }

    std::unique_ptr<testing::StrictMock<MockCpuMonitor>> monitor_ =
        std::make_unique<testing::StrictMock<MockCpuMonitor>>();
    std::unique_ptr<testing::StrictMock<MockMetricsPresenter>> presenter_ =
        std::make_unique<testing::StrictMock<MockMetricsPresenter>>();
};

TEST_F(MonitoringServiceImplTest, RejectsNullMonitor) {
    EXPECT_THROW((void)MonitoringServiceImpl(nullptr, std::move(memory_monitor_),
                                             std::move(process_monitor_), std::move(presenter_)),
                 std::invalid_argument);
}

TEST_F(MonitoringServiceImplTest, RejectsNullPresenter) {
    EXPECT_THROW((void)MonitoringServiceImpl(std::move(monitor_), std::move(memory_monitor_),
                                             std::move(process_monitor_), nullptr),
                 std::invalid_argument);
}

TEST_F(MonitoringServiceImplTest, RejectsNullMemoryMonitor) {
    EXPECT_THROW((void)MonitoringServiceImpl(std::move(monitor_), nullptr,
                                             std::move(process_monitor_), std::move(presenter_)),
                 std::invalid_argument);
}

TEST_F(MonitoringServiceImplTest, PresentsEachAvailableSampleIncludingZero) {
    expectMemorySamples(2);
    expectProcessSamples(2);
    constexpr double kUsage = 42.5;
    EXPECT_CALL(*monitor_, sample())
        .WillOnce(testing::Return(SampleResult(std::optional<double>{kUsage})))
        .WillOnce(testing::Return(SampleResult(std::optional<double>{0.0})));
    testing::InSequence sequence;
    EXPECT_CALL(*presenter_, presentCpuUsage(kUsage));
    EXPECT_CALL(*presenter_, presentCpuUsage(0.0));
    MonitoringServiceImpl service(std::move(monitor_), std::move(memory_monitor_),
                                  std::move(process_monitor_), std::move(presenter_));

    EXPECT_TRUE(service.tick().has_value());
    EXPECT_TRUE(service.tick().has_value());
}

TEST_F(MonitoringServiceImplTest, PresentsMemoryAndProcessesDuringCpuWarmUp) {
    expectMemorySamples(1);
    expectProcessSamples(1);
    EXPECT_CALL(*monitor_, sample()).WillOnce(testing::Return(SampleResult(std::nullopt)));
    EXPECT_CALL(*presenter_, presentCpuUsage(testing::_)).Times(0);
    MonitoringServiceImpl service(std::move(monitor_), std::move(memory_monitor_),
                                  std::move(process_monitor_), std::move(presenter_));

    EXPECT_TRUE(service.tick().has_value());
}

TEST_F(MonitoringServiceImplTest, ReturnsSamplingErrorsWithoutPresenting) {
    EXPECT_CALL(*memory_monitor_, sample()).Times(0);
    EXPECT_CALL(*monitor_, sample())
        .WillOnce(testing::Return(SampleResult(std::unexpected(CpuMonitorError::ReadFailed))))
        .WillOnce(
            testing::Return(SampleResult(std::unexpected(CpuMonitorError::CalculationFailed))));
    EXPECT_CALL(*presenter_, presentCpuUsage(testing::_)).Times(0);
    MonitoringServiceImpl service(std::move(monitor_), std::move(memory_monitor_),
                                  std::move(process_monitor_), std::move(presenter_));

    const auto read_failure = service.tick();
    ASSERT_FALSE(read_failure.has_value());
    EXPECT_EQ(read_failure.error(), MonitoringError::CpuReadFailed);

    const auto calculation_failure = service.tick();
    ASSERT_FALSE(calculation_failure.has_value());
    EXPECT_EQ(calculation_failure.error(), MonitoringError::CpuCalculationFailed);
}

TEST_F(MonitoringServiceImplTest, ContinuesSamplingAfterFailureAndWarmUp) {
    expectMemorySamples(2);
    expectProcessSamples(2);
    constexpr double kUsage = 75.5;
    EXPECT_CALL(*monitor_, sample())
        .WillOnce(testing::Return(SampleResult(std::unexpected(CpuMonitorError::ReadFailed))))
        .WillOnce(testing::Return(SampleResult(std::nullopt)))
        .WillOnce(testing::Return(SampleResult(std::optional<double>{kUsage})));
    EXPECT_CALL(*presenter_, presentCpuUsage(kUsage)).Times(1);
    MonitoringServiceImpl service(std::move(monitor_), std::move(memory_monitor_),
                                  std::move(process_monitor_), std::move(presenter_));

    const auto failure = service.tick();
    ASSERT_FALSE(failure.has_value());
    EXPECT_EQ(failure.error(), MonitoringError::CpuReadFailed);
    EXPECT_TRUE(service.tick().has_value());
    EXPECT_TRUE(service.tick().has_value());
}

struct MemoryFailureCase {
    MemoryMonitorError error;
    MonitoringError expected_error;
};

class MonitoringServiceImplMemoryErrorTest : public MonitoringServiceImplTest,
                                             public testing::WithParamInterface<MemoryFailureCase> {
};

TEST_P(MonitoringServiceImplMemoryErrorTest, ReturnsMemoryErrorWithoutPresentingEitherMetric) {
    EXPECT_CALL(*monitor_, sample())
        .WillOnce(testing::Return(SampleResult(std::optional<double>{50.0})));
    EXPECT_CALL(*memory_monitor_, sample())
        .WillOnce(testing::Return(MemorySampleResult(std::unexpected(GetParam().error))));
    EXPECT_CALL(*presenter_, presentCpuUsage(testing::_)).Times(0);
    EXPECT_CALL(*presenter_, presentMemoryUsage(testing::_)).Times(0);
    MonitoringServiceImpl service(std::move(monitor_), std::move(memory_monitor_),
                                  std::move(process_monitor_), std::move(presenter_));

    const auto result = service.tick();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), GetParam().expected_error);
}

INSTANTIATE_TEST_SUITE_P(MemoryFailures, MonitoringServiceImplMemoryErrorTest,
                         testing::Values(MemoryFailureCase{MemoryMonitorError::ReadFailed,
                                                           MonitoringError::MemoryReadFailed},
                                         MemoryFailureCase{MemoryMonitorError::InvalidStats,
                                                           MonitoringError::MemoryInvalidStats}));

TEST_F(MonitoringServiceImplTest, ReturnsMemoryFailureDuringCpuWarmUp) {
    EXPECT_CALL(*monitor_, sample()).WillOnce(testing::Return(SampleResult(std::nullopt)));
    EXPECT_CALL(*memory_monitor_, sample())
        .WillOnce(
            testing::Return(MemorySampleResult(std::unexpected(MemoryMonitorError::ReadFailed))));
    MonitoringServiceImpl service(std::move(monitor_), std::move(memory_monitor_),
                                  std::move(process_monitor_), std::move(presenter_));

    const auto result = service.tick();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), MonitoringError::MemoryReadFailed);
}

TEST_F(MonitoringServiceImplTest, ContinuesSamplingAfterMemoryFailure) {
    expectProcessSamples(1);
    auto* presenter = presenter_.get();
    EXPECT_CALL(*monitor_, sample())
        .WillOnce(testing::Return(SampleResult(std::optional<double>{50.0})))
        .WillOnce(testing::Return(SampleResult(std::optional<double>{25.0})));
    EXPECT_CALL(*memory_monitor_, sample())
        .WillOnce(
            testing::Return(MemorySampleResult(std::unexpected(MemoryMonitorError::ReadFailed))))
        .WillOnce(testing::Return(
            MemorySampleResult(MemoryUsage{.used_kb = 250, .total_kb = 1000, .percentage = 25.0})));
    MonitoringServiceImpl service(std::move(monitor_), std::move(memory_monitor_),
                                  std::move(process_monitor_), std::move(presenter_));

    const auto failure = service.tick();
    ASSERT_FALSE(failure.has_value());
    EXPECT_EQ(failure.error(), MonitoringError::MemoryReadFailed);

    EXPECT_CALL(*presenter, presentCpuUsage(25.0));
    EXPECT_CALL(*presenter,
                presentMemoryUsage(testing::AllOf(testing::Field(&MemoryUsage::used_kb, 250),
                                                  testing::Field(&MemoryUsage::total_kb, 1000),
                                                  testing::Field(&MemoryUsage::percentage, 25.0))));
    EXPECT_TRUE(service.tick().has_value());
}

TEST_F(MonitoringServiceImplTest, RejectsNullProcessMonitor) {
    EXPECT_THROW((void)MonitoringServiceImpl(std::move(monitor_), std::move(memory_monitor_),
                                             nullptr, std::move(presenter_)),
                 std::invalid_argument);
}

TEST_F(MonitoringServiceImplTest, PresentsProcessesAfterCpuAndMemoryWithoutChangingTheirDetails) {
    const MemoryUsage memory{.used_kb = 250, .total_kb = 1000, .percentage = 25.0};
    const std::vector<ProcessInfo> processes{
        {.pid = 42, .name = "pulse", .state = 'R', .memory_kb = 1024},
        {.pid = 7, .name = "unknown", .state = 'S', .memory_kb = std::nullopt},
    };
    testing::InSequence sequence;
    EXPECT_CALL(*monitor_, sample())
        .WillOnce(testing::Return(SampleResult(std::optional<double>{50.0})));
    EXPECT_CALL(*memory_monitor_, sample()).WillOnce(testing::Return(MemorySampleResult(memory)));
    EXPECT_CALL(*process_monitor_, sample())
        .WillOnce(testing::Return(ProcessSampleResult(processes)));
    EXPECT_CALL(*presenter_, presentCpuUsage(50.0));
    EXPECT_CALL(*presenter_, presentMemoryUsage(testing::_));
    EXPECT_CALL(*presenter_,
                presentProcesses(testing::ElementsAre(
                    testing::FieldsAre(42, "pulse", 'R', std::optional<std::uint64_t>(1024)),
                    testing::FieldsAre(7, "unknown", 'S', std::nullopt))));
    MonitoringServiceImpl service(std::move(monitor_), std::move(memory_monitor_),
                                  std::move(process_monitor_), std::move(presenter_));

    EXPECT_TRUE(service.tick().has_value());
}

class MonitoringServiceImplProcessErrorTest : public MonitoringServiceImplTest,
                                              public testing::WithParamInterface<bool> {};

TEST_P(MonitoringServiceImplProcessErrorTest, ReturnsProcessErrorWithoutOutputAndRecovers) {
    const auto cpu_usage = GetParam() ? std::optional<double>{50.0} : std::nullopt;
    EXPECT_CALL(*monitor_, sample())
        .WillOnce(testing::Return(SampleResult(cpu_usage)))
        .WillOnce(testing::Return(SampleResult(std::optional<double>{25.0})));
    const MemoryUsage memory{.used_kb = 250, .total_kb = 1000, .percentage = 25.0};
    EXPECT_CALL(*memory_monitor_, sample())
        .Times(2)
        .WillRepeatedly(testing::Return(MemorySampleResult(memory)));
    EXPECT_CALL(*process_monitor_, sample())
        .WillOnce(testing::Return(
            ProcessSampleResult(std::unexpected(ProcessReadError::DirectoryReadFailed))))
        .WillOnce(testing::Return(ProcessSampleResult{}));
    auto& presenter = *presenter_;
    MonitoringServiceImpl service(std::move(monitor_), std::move(memory_monitor_),
                                  std::move(process_monitor_), std::move(presenter_));

    // StrictMock rejects any presentation before all three samples succeed.
    const auto failure = service.tick();
    ASSERT_FALSE(failure.has_value());
    EXPECT_EQ(failure.error(), MonitoringError::ProcessReadFailed);

    EXPECT_CALL(presenter, presentCpuUsage(25.0));
    EXPECT_CALL(presenter, presentMemoryUsage(testing::_));
    EXPECT_CALL(presenter, presentProcesses(testing::IsEmpty()));
    EXPECT_TRUE(service.tick().has_value());
}

INSTANTIATE_TEST_SUITE_P(CpuWarmUpAndAvailableUsage, MonitoringServiceImplProcessErrorTest,
                         testing::Bool());

}  // namespace
}  // namespace pulse
