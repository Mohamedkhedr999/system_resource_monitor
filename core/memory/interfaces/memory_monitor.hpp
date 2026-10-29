#ifndef SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_INTERFACES_MEMORY_MONITOR_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_INTERFACES_MEMORY_MONITOR_HPP

#include <cstdint>
#include <expected>

#include "core/memory/memory_usage.hpp"

namespace pulse {

enum class MemoryMonitorError : std::uint8_t { ReadFailed, InvalidStats };

class IMemoryMonitor {
   public:
    virtual ~IMemoryMonitor() = default;

    [[nodiscard]]
    virtual std::expected<MemoryUsage, MemoryMonitorError> sample() = 0;
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_INTERFACES_MEMORY_MONITOR_HPP
