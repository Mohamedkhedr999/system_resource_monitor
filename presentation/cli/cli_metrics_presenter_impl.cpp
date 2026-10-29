#include "presentation/cli/cli_metrics_presenter_impl.hpp"

#include <iomanip>
#include <ostream>

namespace pulse {

CliMetricsPresenterImpl::CliMetricsPresenterImpl(std::ostream& output)
    : IMetricsPresenter(), output_(output) {}

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

}  // namespace pulse
