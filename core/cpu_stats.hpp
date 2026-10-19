#ifndef SYSTEM_RESOURCE_MONITOR_CORE_CPU_STATS_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_CPU_STATS_HPP

#include <cstdint>

namespace pulse {

// Aggregate CPU time counters reported by /proc/stat, in USER_HZ ticks.
struct CpuStats {
    // Time spent executing in user mode.
    std::uint64_t user{0};
    // Time spent executing user processes with a positive nice value (lower priority).
    std::uint64_t nice{0};
    // Time spent executing in kernel mode.
    std::uint64_t system{0};
    // Time spent in the idle task.
    std::uint64_t idle{0};
    // Time accounted as waiting for I/O; this counter can sometimes decrease.
    std::uint64_t iowait{0};
    // Time spent servicing hardware interrupts.
    std::uint64_t irq{0};
    // Time spent servicing software interrupts.
    std::uint64_t softirq{0};
    // Time a virtual CPU was unavailable while the hypervisor ran other workloads.
    std::uint64_t steal{0};
};

}  // namespace pulse
#endif
