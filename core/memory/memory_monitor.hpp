#ifndef SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_MEMORY_MONITOR_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_MEMORY_MONITOR_HPP

#include <cstdint>
#include <expected>
#include <memory>

#include "core/memory/interfaces/memory_reader.hpp"
#include "core/memory/memory_usage.hpp"

namespace pulse {

enum class MemoryMonitorError :std::uint8_t { ReadFailed, InvalidStats };

class MemoryMonitor {
   public:
    explicit MemoryMonitor(std::unique_ptr<IMemoryReader> reader);

    [[nodiscard]]
    std::expected<MemoryUsage, MemoryMonitorError> sample();

   private:
    std::unique_ptr<IMemoryReader> reader_;
};

}  // namespace pulse

#endif
