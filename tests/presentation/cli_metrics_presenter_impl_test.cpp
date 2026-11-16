#include "presentation/cli/cli_metrics_presenter_impl.hpp"

#include <iomanip>
#include <optional>
#include <sstream>
#include <vector>

#include "core/process/process_info.hpp"
#include "gtest/gtest.h"

namespace pulse {
namespace {

TEST(CliMetricsPresenterImplTest, BeginsFrameWithCursorHomeAndClearDisplay) {
    constexpr double kCpuUsage = 25.0;
    constexpr MemoryUsage kMemoryUsage{.used_kb = 1048576, .total_kb = 4194304, .percentage = 25.0};
    std::ostringstream output;
    CliMetricsPresenterImpl presenter(output);

    presenter.beginFrame();
    presenter.presentCpuUsage(kCpuUsage);
    presenter.presentMemoryUsage(kMemoryUsage);
    presenter.presentProcesses({});

    EXPECT_EQ(output.str(),
              "\033[H\033[2J"
              "CPU Usage: 25.00%\n"
              "Memory Usage: 1.00 / 4.00 GiB (25.00%)\n"
              "\nTOP 0 PROCESSES BY MEMORY\n"
              "PID       NAME                  STATE    CPU%       MEMORY\n");
}

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

TEST(CliMetricsPresenterImplTest, PresentsProcessColumnsAndConvertsKibToMib) {
    const std::vector<ProcessInfo> processes{
        {.pid = 1234, .name = "firefox", .state = 'S', .memory_kb = 870400, .cpu_percentage = 12.5},
        {.pid = 3789, .name = "pulse", .state = 'R', .memory_kb = 12800, .cpu_percentage = 3.2},
    };
    std::ostringstream output;
    CliMetricsPresenterImpl presenter(output);

    presenter.presentProcesses(processes);

    EXPECT_EQ(output.str(),
              "\nTOP 2 PROCESSES BY MEMORY\n"
              "PID       NAME                  STATE    CPU%       MEMORY\n"
              "1234      firefox               S        12.50%     850.00 MiB\n"
              "3789      pulse                 R        3.20%      12.50 MiB\n");
}

TEST(CliMetricsPresenterImplTest, DistinguishesUnknownMemoryFromZeroAndPreservesInputOrder) {
    const std::vector<ProcessInfo> processes{
        {.pid = 9, .name = "unknown", .state = 'S', .memory_kb = std::nullopt},
        {.pid = 2, .name = "zero", .state = 'R', .memory_kb = 0},
    };
    std::ostringstream output;
    CliMetricsPresenterImpl presenter(output);

    presenter.presentProcesses(processes);

    EXPECT_EQ(output.str(),
              "\nTOP 2 PROCESSES BY MEMORY\n"
              "PID       NAME                  STATE    CPU%       MEMORY\n"
              "9         unknown               S        N/A        N/A\n"
              "2         zero                  R        N/A        0.00 MiB\n");
}

TEST(CliMetricsPresenterImplTest, DistinguishesZeroCpuFromUnknownAndAllowsMulticoreUsage) {
    const std::vector<ProcessInfo> processes{
        {.pid = 1, .name = "idle", .state = 'S', .memory_kb = 0, .cpu_percentage = 0.0},
        {.pid = 2, .name = "busy", .state = 'R', .memory_kb = 1024, .cpu_percentage = 150.0},
    };
    std::ostringstream output;
    CliMetricsPresenterImpl presenter(output);

    presenter.presentProcesses(processes);

    EXPECT_EQ(output.str(),
              "\nTOP 2 PROCESSES BY MEMORY\n"
              "PID       NAME                  STATE    CPU%       MEMORY\n"
              "1         idle                  S        0.00%      0.00 MiB\n"
              "2         busy                  R        150.00%    1.00 MiB\n");
}

TEST(CliMetricsPresenterImplTest, CpuColumnPreservesOutputPrecisionAndFloatingPointFormat) {
    std::ostringstream output;
    output << std::scientific << std::setprecision(4);
    CliMetricsPresenterImpl presenter(output);

    presenter.presentProcesses({{.pid = 1,
                                 .name = "worker",
                                 .state = 'R',
                                 .memory_kb = std::nullopt,
                                 .cpu_percentage = 12.5}});

    EXPECT_EQ(output.str(),
              "\nTOP 1 PROCESSES BY MEMORY\n"
              "PID       NAME                  STATE    CPU%       MEMORY\n"
              "1         worker                R        12.50%     N/A\n");
    EXPECT_EQ(output.precision(), 4);
    EXPECT_EQ(output.flags() & std::ios_base::floatfield, std::ios_base::scientific);
}

TEST(CliMetricsPresenterImplTest, AppendsEmptyProcessTableAfterCpuAndMemory) {
    std::ostringstream output;
    CliMetricsPresenterImpl presenter(output);

    presenter.presentCpuUsage(2.45);
    presenter.presentMemoryUsage({.used_kb = 1048576, .total_kb = 4194304, .percentage = 25.0});
    presenter.presentProcesses({});

    EXPECT_EQ(output.str(),
              "CPU Usage: 2.45%\n"
              "Memory Usage: 1.00 / 4.00 GiB (25.00%)\n"
              "\nTOP 0 PROCESSES BY MEMORY\n"
              "PID       NAME                  STATE    CPU%       MEMORY\n");
}

}  // namespace
}  // namespace pulse
