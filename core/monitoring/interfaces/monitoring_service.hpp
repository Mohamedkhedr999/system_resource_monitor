#ifndef SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_INTERFACES_MONITORING_SERVICE_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_INTERFACES_MONITORING_SERVICE_HPP

#include <cstdint>
#include <expected>

namespace pulse {

enum class MonitoringError : std::uint8_t {
    CpuReadFailed,
    CpuCalculationFailed,
    MemoryReadFailed,
    MemoryInvalidStats,
    ProcessReadFailed
};

class IMonitoringService {
   public:
    virtual ~IMonitoringService() = default;

    // Prints memory usage, processes, and CPU usage when available.
    // The first tick omits CPU usage because it needs two samples.
    // If any monitor fails, returns an error without printing any metrics.
    [[nodiscard]]
    virtual std::expected<void, MonitoringError> tick() = 0;
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_INTERFACES_MONITORING_SERVICE_HPP
