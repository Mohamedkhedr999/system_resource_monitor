#include "presentation/cli/cli_metrics_presenter.hpp"

#include <sstream>

#include "gtest/gtest.h"

namespace pulse {
namespace {

TEST(CliMetricsPresenterTest, PresentsFractionalUsageWithLabelUnitAndNewline) {
    constexpr double kFractionalUsage = 42.5;
    std::ostringstream output;
    CliMetricsPresenter presenter(output);

    presenter.presentCpuUsage(kFractionalUsage);

    EXPECT_EQ(output.str(), "CPU Usage: 42.50%\n");
}

TEST(CliMetricsPresenterTest, PresentsZeroUsage) {
    std::ostringstream output;
    CliMetricsPresenter presenter(output);

    presenter.presentCpuUsage(0.0);

    EXPECT_EQ(output.str(), "CPU Usage: 0.00%\n");
}

TEST(CliMetricsPresenterTest, PresentsFullUsage) {
    std::ostringstream output;
    CliMetricsPresenter presenter(output);

    presenter.presentCpuUsage(100.0);

    EXPECT_EQ(output.str(), "CPU Usage: 100.00%\n");
}

TEST(CliMetricsPresenterTest, AppendsSuccessiveSamplesOnSeparateLines) {
    constexpr double kFirstSample = 25.0;
    constexpr double kSecondSample = 75.5;
    std::ostringstream output;
    output << "Existing output\n";
    CliMetricsPresenter presenter(output);

    presenter.presentCpuUsage(kFirstSample);
    presenter.presentCpuUsage(kSecondSample);

    EXPECT_EQ(output.str(), "Existing output\nCPU Usage: 25.00%\nCPU Usage: 75.50%\n");
}

}  // namespace
}  // namespace pulse
