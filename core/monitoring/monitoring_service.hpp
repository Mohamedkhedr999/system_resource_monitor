#ifndef SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_MONITORING_SERVICE_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_MONITORING_SERVICE_HPP

#include <expected>
#include <memory>

#include "core/cpu/interfaces/cpu_monitor.hpp"
#include "core/monitoring/interfaces/metrics_presenter.hpp"

namespace pulse {

class MonitoringService {
   public:
    MonitoringService(std::unique_ptr<ICpuMonitor> cpu_monitor,
                      std::unique_ptr<IMetricsPresenter> presenter);

    // Presents available usage; warm-up succeeds, sampling errors are returned.
    [[nodiscard]]
    std::expected<void, CpuMonitorError> tick();

   private:
    std::unique_ptr<ICpuMonitor> cpu_monitor_;
    std::unique_ptr<IMetricsPresenter> presenter_;
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_MONITORING_SERVICE_HPP
