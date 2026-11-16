#include "presentation/cli/cli_metrics_presenter_impl.hpp"

#include <iomanip>
#include <ostream>
#include <sstream>

namespace pulse {

CliMetricsPresenterImpl::CliMetricsPresenterImpl(std::ostream& output)
    : IMetricsPresenter(), output_(output) {}

void CliMetricsPresenterImpl::beginFrame() { output_ << "\033[H\033[2J"; }

void CliMetricsPresenterImpl::presentCpuUsage(double percentage) {
    output_ << std::fixed << std::setprecision(2) << "CPU Usage: " << percentage << "%\n";
}

void CliMetricsPresenterImpl::presentMemoryUsage(const MemoryUsage& usage) {
    // /proc/meminfo reports kB in units of 1024 bytes (KiB).
    constexpr double kKibPerGib = 1024.0 * 1024.0;
    const double used_gib = static_cast<double>(usage.used_kb) / kKibPerGib;
    const double total_gib = static_cast<double>(usage.total_kb) / kKibPerGib;

    output_ << std::fixed << std::setprecision(2) << "Memory Usage: " << used_gib << " / "
            << total_gib << " GiB (" << usage.percentage << "%)\n";
}

void CliMetricsPresenterImpl::presentProcesses(const std::vector<ProcessInfo>& processes) {
    constexpr double kKibPerMib = 1024.0;
    constexpr int kPidWidth = 10;
    constexpr int kNameWidth = 22;
    constexpr int kStateWidth = 9;
    constexpr int kCpuWidth = 11;

    output_ << "\nTOP " << processes.size() << " PROCESSES BY MEMORY\n";
    output_ << std::left << std::setw(kPidWidth) << "PID" << std::setw(kNameWidth) << "NAME"
            << std::setw(kStateWidth) << "STATE" << std::setw(kCpuWidth) << "CPU%" << "MEMORY\n";

    for (const auto& process : processes) {
        output_ << std::left << std::setw(kPidWidth) << process.pid << std::setw(kNameWidth)
                << process.name << std::setw(kStateWidth) << process.state;

        if (process.cpu_percentage) {
            std::ostringstream cpu_stream;
            cpu_stream << std::fixed << std::setprecision(2) << *process.cpu_percentage << '%';
            output_ << std::setw(kCpuWidth) << cpu_stream.str();
        } else {
            output_ << std::setw(kCpuWidth) << "N/A";
        }

        if (process.memory_kb) {
            const double memory_mib = static_cast<double>(*process.memory_kb) / kKibPerMib;
            output_ << std::fixed << std::setprecision(2) << memory_mib << " MiB";
        } else {
            output_ << "N/A";
        }

        output_ << '\n';
    }
}

}  // namespace pulse
