#ifndef SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_CPU_USAGE_CALCULATOR_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_CPU_USAGE_CALCULATOR_HPP

#include <chrono>
#include <cstdint>
#include <expected>

#include "core/process/process_cpu_stats.hpp"

namespace pulse {

enum class ProcessCpuCalculationError {
    InvalidSampleOrder,
    ProcessChanged,
    InvalidElapsedTime,
    InvalidClockTicks
};

class ProcessCpuUsageCalculator {
   public:
    [[nodiscard]]
    static std::expected<double, ProcessCpuCalculationError> calculate(
        const ProcessCpuStats& previous, const ProcessCpuStats& current,
        std::chrono::steady_clock::duration elapsed, std::uint64_t clock_ticks_per_second);
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_CPU_USAGE_CALCULATOR_HPP
