#ifndef SYSTEM_RESOURCE_MONITOR_CORE_CPU_INTERFACES_CPU_MONITOR_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_CPU_INTERFACES_CPU_MONITOR_HPP

#include <cstdint>
#include <expected>
#include <optional>

namespace pulse {

enum class CpuMonitorError : std::uint8_t { ReadFailed, CalculationFailed };

class ICpuMonitor {
   public:
    ICpuMonitor() = default;
    virtual ~ICpuMonitor() = default;

    ICpuMonitor(const ICpuMonitor&) = delete;
    ICpuMonitor& operator=(const ICpuMonitor&) = delete;
    ICpuMonitor(ICpuMonitor&&) = delete;
    ICpuMonitor& operator=(ICpuMonitor&&) = delete;

    [[nodiscard]]
    virtual std::expected<std::optional<double>, CpuMonitorError> sample() = 0;
};

}  // namespace pulse

#endif
