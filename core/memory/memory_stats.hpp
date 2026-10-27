#ifndef SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_MEMORY_STATS_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_MEMORY_STATS_HPP

#include <cstdint>

namespace pulse {

struct MemoryStats {
    std::uint64_t total_kb{};
    std::uint64_t available_kb{};
};

}  // namespace pulse

#endif
