#include "presentation/cli/cli_metrics_presenter_impl.hpp"

#include <sstream>

#include "gtest/gtest.h"

namespace pulse {
namespace {

TEST(CliMetricsPresenterImplTest, PresentsFractionalUsageWithLabelUnitAndNewline) {
    constexpr double kFractionalUsage = 42.5;
    std::ostringstream output;
    CliMetricsPresenterImpl presenter(output);

    presenter.presentCpuUsage(kFractionalUsage);

    EXPECT_EQ(output.str(), "CPU Usage: 42.50%\n");
}

TEST(CliMetricsPresenterImplTest, PresentsZeroUsage) {
    std::ostringstream output;
    CliMetricsPresenterImpl presenter(output);

    presenter.presentCpuUsage(0.0);

    EXPECT_EQ(output.str(), "CPU Usage: 0.00%\n");
}

TEST(CliMetricsPresenterImplTest, PresentsFullUsage) {
    std::ostringstream output;
    CliMetricsPresenterImpl presenter(output);

    presenter.presentCpuUsage(100.0);

    EXPECT_EQ(output.str(), "CPU Usage: 100.00%\n");
}

TEST(CliMetricsPresenterImplTest, AppendsSuccessiveSamplesOnSeparateLines) {
    constexpr double kFirstSample = 25.0;
    constexpr double kSecondSample = 75.5;
    std::ostringstream output;
    output << "Existing output\n";
    CliMetricsPresenterImpl presenter(output);

    presenter.presentCpuUsage(kFirstSample);
    presenter.presentCpuUsage(kSecondSample);

    EXPECT_EQ(output.str(), "Existing output\nCPU Usage: 25.00%\nCPU Usage: 75.50%\n");
}

TEST(CliMetricsPresenterImplTest, PresentsMemoryAmountsAndFractionalPercentage) {
    constexpr MemoryUsage kUsage{.used_kb = 4456448, .total_kb = 10485760, .percentage = 42.5};
    std::ostringstream output;
    CliMetricsPresenterImpl presenter(output);

    presenter.presentMemoryUsage(kUsage);

    EXPECT_EQ(output.str(), "Memory Usage: 4.25 / 10.00 GiB (42.50%)\n");
}

TEST(CliMetricsPresenterImplTest, PresentsZeroMemoryUsage) {
    constexpr MemoryUsage kUsage{.used_kb = 0, .total_kb = 1048576, .percentage = 0.0};
    std::ostringstream output;
    CliMetricsPresenterImpl presenter(output);

    presenter.presentMemoryUsage(kUsage);

    EXPECT_EQ(output.str(), "Memory Usage: 0.00 / 1.00 GiB (0.00%)\n");
}

TEST(CliMetricsPresenterImplTest, AppendsCpuAndFullMemoryUsageOnSeparateLines) {
    constexpr MemoryUsage kUsage{.used_kb = 1048576, .total_kb = 1048576, .percentage = 100.0};
    std::ostringstream output;
    CliMetricsPresenterImpl presenter(output);

    presenter.presentCpuUsage(25.0);
    presenter.presentMemoryUsage(kUsage);

    EXPECT_EQ(output.str(), "CPU Usage: 25.00%\nMemory Usage: 1.00 / 1.00 GiB (100.00%)\n");
}

}  // namespace
}  // namespace pulse
