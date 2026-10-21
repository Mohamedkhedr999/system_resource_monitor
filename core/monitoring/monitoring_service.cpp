#include "core/monitoring/monitoring_service.hpp"

#include <stdexcept>
#include <utility>

namespace pulse {

MonitoringService::MonitoringService(std::unique_ptr<ICpuMonitor> cpu_monitor,
                                     std::unique_ptr<IMetricsPresenter> presenter)
    : cpu_monitor_(std::move(cpu_monitor)), presenter_(std::move(presenter)) {
    if (!cpu_monitor_ || !presenter_) {
        throw std::invalid_argument("MonitoringService requires a CPU monitor and presenter");
    }
}

std::expected<void, CpuMonitorError> MonitoringService::tick() {
    const auto usage = cpu_monitor_->sample();
    if (!usage) {
        return std::unexpected(usage.error());
    }
    if (usage->has_value()) {
        presenter_->presentCpuUsage(**usage);
    }
    return {};
}

}  // namespace pulse
