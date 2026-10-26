#ifndef SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_INTERFACES_MONITORING_SERVICE_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_INTERFACES_MONITORING_SERVICE_HPP

#include <expected>

#include "core/cpu/interfaces/cpu_monitor.hpp"

namespace pulse {

class IMonitoringService {
   public:
    virtual ~IMonitoringService() = default;

    // Presents available usage; warm-up succeeds, sampling errors are returned.
    [[nodiscard]]
    virtual std::expected<void, CpuMonitorError> tick() = 0;
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_INTERFACES_MONITORING_SERVICE_HPP
