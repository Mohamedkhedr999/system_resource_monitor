#include "core/monitoring/monitoring_service_impl.hpp"

#include <stdexcept>
#include <utility>

namespace pulse {

MonitoringServiceImpl::MonitoringServiceImpl(std::unique_ptr<ICpuMonitor> cpu_monitor,
                                             std::unique_ptr<IMemoryMonitor> memory_monitor,
                                             std::unique_ptr<IMetricsPresenter> presenter)
    : cpu_monitor_(std::move(cpu_monitor)),
      memory_monitor_(std::move(memory_monitor)),
      presenter_(std::move(presenter)) {
    if (!cpu_monitor_ || !memory_monitor_ || !presenter_) {
        throw std::invalid_argument(
            "MonitoringServiceImpl requires CPU and memory monitors and a presenter");
    }
}

std::expected<void, MonitoringError> MonitoringServiceImpl::tick() {
    const auto usage = cpu_monitor_->sample();
    if (!usage) {
        return std::unexpected(usage.error() == CpuMonitorError::ReadFailed
                                   ? MonitoringError::CpuReadFailed
                                   : MonitoringError::CpuCalculationFailed);
    }
    const auto memory_usage = memory_monitor_->sample();
    if (!memory_usage) {
        return std::unexpected(memory_usage.error() == MemoryMonitorError::ReadFailed
                                   ? MonitoringError::MemoryReadFailed
                                   : MonitoringError::MemoryInvalidStats);
    }
    if (usage->has_value()) {
        presenter_->presentCpuUsage(**usage);
    }
    presenter_->presentMemoryUsage(*memory_usage);
    return {};
}

}  // namespace pulse
