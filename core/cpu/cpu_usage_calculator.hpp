#ifndef SYSTEM_RESOURCE_MONITOR_CORE_CPU_CPU_USAGE_CALCULATOR_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_CPU_CPU_USAGE_CALCULATOR_HPP

#include <cstdint>
#include <expected>

#include "core/cpu/cpu_stats.hpp"

namespace pulse {

enum class CpuCalculationError : std::uint8_t {
    InvalidSampleOrder,
    ZeroElapsedTime,
    CounterOverflow,
};

class CpuUsageCalculator {
   public:
    [[nodiscard]]
    static std::expected<double, CpuCalculationError> calculate(const CpuStats& previous,
                                                                const CpuStats& current);
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_CPU_CPU_USAGE_CALCULATOR_HPP
