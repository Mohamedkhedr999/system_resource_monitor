#ifndef SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_MONITORING_SERVICE_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_MONITORING_SERVICE_HPP

#include <expected>
#include <memory>

#include "core/cpu/interfaces/cpu_monitor.hpp"
#include "core/monitoring/interfaces/metrics_presenter.hpp"
#include "core/monitoring/interfaces/monitoring_service.hpp"

namespace pulse {

class MonitoringService final : public IMonitoringService {
   public:
    MonitoringService(std::unique_ptr<ICpuMonitor> cpu_monitor,
                      std::unique_ptr<IMetricsPresenter> presenter);

    [[nodiscard]]
    std::expected<void, CpuMonitorError> tick() override;

   private:
    std::unique_ptr<ICpuMonitor> cpu_monitor_;
    std::unique_ptr<IMetricsPresenter> presenter_;
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_MONITORING_SERVICE_HPP
