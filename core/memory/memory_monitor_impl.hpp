#ifndef SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_MEMORY_MONITOR_IMPL_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_MEMORY_MONITOR_IMPL_HPP

#include <expected>
#include <memory>

#include "core/memory/interfaces/memory_monitor.hpp"
#include "core/memory/interfaces/memory_reader.hpp"
#include "core/memory/memory_usage.hpp"

namespace pulse {

class MemoryMonitorImpl final : public IMemoryMonitor {
   public:
    explicit MemoryMonitorImpl(std::unique_ptr<IMemoryReader> reader);

    [[nodiscard]]
    std::expected<MemoryUsage, MemoryMonitorError> sample() override;

   private:
    std::unique_ptr<IMemoryReader> reader_;
};

}  // namespace pulse

#endif
