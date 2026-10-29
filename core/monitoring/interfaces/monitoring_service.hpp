#ifndef SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_INTERFACES_MONITORING_SERVICE_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_INTERFACES_MONITORING_SERVICE_HPP

#include <cstdint>
#include <expected>

namespace pulse {

enum class MonitoringError : std::uint8_t {
    CpuReadFailed,
    CpuCalculationFailed,
    MemoryReadFailed,
    MemoryInvalidStats
};

class IMonitoringService {
   public:
    virtual ~IMonitoringService() = default;

    // Prints memory usage and CPU usage when available.
    // The first tick prints only memory because CPU usage needs two samples.
    // If either monitor fails, returns an error without printing any metrics.
    [[nodiscard]]
    virtual std::expected<void, MonitoringError> tick() = 0;
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_INTERFACES_MONITORING_SERVICE_HPP
