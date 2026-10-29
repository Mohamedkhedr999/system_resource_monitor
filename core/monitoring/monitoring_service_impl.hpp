#ifndef SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_MONITORING_SERVICE_IMPL_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_MONITORING_SERVICE_IMPL_HPP

#include <expected>
#include <memory>

#include "core/cpu/interfaces/cpu_monitor.hpp"
#include "core/memory/interfaces/memory_monitor.hpp"
#include "core/monitoring/interfaces/metrics_presenter.hpp"
#include "core/monitoring/interfaces/monitoring_service.hpp"

namespace pulse {

class MonitoringServiceImpl final : public IMonitoringService {
   public:
    MonitoringServiceImpl(std::unique_ptr<ICpuMonitor> cpu_monitor,
                          std::unique_ptr<IMemoryMonitor> memory_monitor,
                          std::unique_ptr<IMetricsPresenter> presenter);

    [[nodiscard]]
    std::expected<void, MonitoringError> tick() override;

   private:
    std::unique_ptr<ICpuMonitor> cpu_monitor_;
    std::unique_ptr<IMemoryMonitor> memory_monitor_;
    std::unique_ptr<IMetricsPresenter> presenter_;
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_MONITORING_SERVICE_IMPL_HPP
