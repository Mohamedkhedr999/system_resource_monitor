#include "core/cpu/cpu_usage_calculator.hpp"

#include <array>
#include <limits>

namespace pulse {

namespace {

[[nodiscard]]
std::expected<std::uint64_t, CpuCalculationError> checkedAdd(std::uint64_t a, std::uint64_t b) {
    constexpr auto max = std::numeric_limits<std::uint64_t>::max();

    if (a > max - b) {
        return std::unexpected(CpuCalculationError::CounterOverflow);
    }

    return a + b;
}
}  // namespace

std::expected<double, CpuCalculationError> CpuUsageCalculator::calculate(const CpuStats& previous,
                                                                         const CpuStats& current) {
    // Step 1: Validate that no CPU counter has decreased.
    if (current.user < previous.user || current.nice < previous.nice ||
        current.system < previous.system || current.idle < previous.idle ||
        current.iowait < previous.iowait || current.irq < previous.irq ||
        current.softirq < previous.softirq || current.steal < previous.steal) {
        return std::unexpected(CpuCalculationError::InvalidSampleOrder);
    }

    // Step 2: Calculate individual counter differences.
    const auto user_delta = current.user - previous.user;
    const auto nice_delta = current.nice - previous.nice;
    const auto system_delta = current.system - previous.system;
    const auto idle_delta = current.idle - previous.idle;
    const auto iowait_delta = current.iowait - previous.iowait;
    const auto irq_delta = current.irq - previous.irq;
    const auto softirq_delta = current.softirq - previous.softirq;
    const auto steal_delta = current.steal - previous.steal;

    // Step 3: Store the differences in an array.
    const std::array<std::uint64_t, 8> deltas{user_delta,   nice_delta, system_delta,  idle_delta,
                                              iowait_delta, irq_delta,  softirq_delta, steal_delta};

    // Step 4: Calculate total elapsed CPU time safely.
    std::uint64_t total_delta = 0;

    for (const auto delta : deltas) {
        const auto result = checkedAdd(total_delta, delta);

        if (!result) {
            return std::unexpected(result.error());
        }

        total_delta = *result;
    }

    // Step 5: Calculate idle time safely.
    const auto idle_result = checkedAdd(idle_delta, iowait_delta);

    if (!idle_result) {
        return std::unexpected(idle_result.error());
    }

    const auto total_idle_delta = *idle_result;

    // Step 6: Avoid division by zero.
    if (total_delta == 0) {
        return std::unexpected(CpuCalculationError::ZeroElapsedTime);
    }

    // Step 7: Validate idle time.
    if (total_idle_delta > total_delta) {
        return std::unexpected(CpuCalculationError::InvalidSampleOrder);
    }

    // Step 8: Calculate CPU usage percentage.
    constexpr double percentage_scale = 100.0;

    return percentage_scale * static_cast<double>(total_delta - total_idle_delta) /
           static_cast<double>(total_delta);
}

}  // namespace pulse
