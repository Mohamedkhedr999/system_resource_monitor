
#include "core/process/process_cpu_usage_calculator.hpp"

#include <chrono>

namespace pulse {

std::expected<double, ProcessCpuCalculationError> ProcessCpuUsageCalculator::calculate(
    const ProcessCpuStats& previous, const ProcessCpuStats& current,
    std::chrono::steady_clock::duration elapsed, std::uint64_t clock_ticks_per_second) {
    // Never compare samples belonging to different processes.
    // start_time_ticks also protects against PID reuse.
    if (previous.pid != current.pid || previous.start_time_ticks != current.start_time_ticks) {
        return std::unexpected(ProcessCpuCalculationError::ProcessChanged);
    }

    // CPU counters should not decrease for the same process.
    if (current.user_ticks < previous.user_ticks || current.system_ticks < previous.system_ticks) {
        return std::unexpected(ProcessCpuCalculationError::InvalidSampleOrder);
    }

    if (elapsed <= std::chrono::steady_clock::duration::zero()) {
        return std::unexpected(ProcessCpuCalculationError::InvalidElapsedTime);
    }

    if (clock_ticks_per_second == 0) {
        return std::unexpected(ProcessCpuCalculationError::InvalidClockTicks);
    }

    const auto user_delta = current.user_ticks - previous.user_ticks;

    const auto system_delta = current.system_ticks - previous.system_ticks;

    const double elapsed_seconds = std::chrono::duration<double>(elapsed).count();

    const double total_cpu_seconds =
        (static_cast<double>(user_delta) + static_cast<double>(system_delta)) /
        static_cast<double>(clock_ticks_per_second);

    const double cpu_percentage = (total_cpu_seconds / elapsed_seconds) * 100.0;

    return cpu_percentage;
}

}  // namespace pulse
