#ifndef SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_MEMORY_USAGE_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_MEMORY_USAGE_HPP

#include <cstdint>

namespace pulse {

struct MemoryUsage {
    std::uint64_t used_kb{};
    std::uint64_t total_kb{};
    double percentage{};
};

}  // namespace pulse

#endif
