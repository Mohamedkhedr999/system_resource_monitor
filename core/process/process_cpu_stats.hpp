#ifndef SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_CPU_STATS_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_CPU_STATS_HPP

#include <cstdint>

namespace pulse {

struct ProcessCpuStats {
    std::int32_t pid{};
    std::uint64_t user_ticks{};
    std::uint64_t system_ticks{};
    std::uint64_t start_time_ticks{};
};

}  // namespace pulse

#endif
