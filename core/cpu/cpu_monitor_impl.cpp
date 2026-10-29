#include "core/cpu/cpu_monitor_impl.hpp"

#include <optional>
#include <stdexcept>
#include <utility>

#include "core/cpu/cpu_usage_calculator.hpp"

namespace pulse {

CpuMonitorImpl::CpuMonitorImpl(std::shared_ptr<ICpuReader> reader)
    : ICpuMonitor(), reader_(std::move(reader)), previous_(std::nullopt) {
    if (!reader_) {
        throw std::invalid_argument("CpuMonitorImpl requires a valid ICpuReader");
    }
}

std::expected<std::optional<double>, CpuMonitorError> CpuMonitorImpl::sample() {
    const auto current = reader_->read();

    // Step 1: Handle a failed CPU reading.
    if (!current) {
        return std::unexpected(CpuMonitorError::ReadFailed);
    }

    // Step 2: First reading: nothing to compare yet.
    if (!previous_) {
        previous_ = *current;
        return std::optional<double>{};
    }

    // Step 3: Calculate CPU usage.
    const auto usage = CpuUsageCalculator::calculate(*previous_, *current);

    // Step 4: Save the latest reading.
    previous_ = *current;

    // Step 5: Handle calculation failure.
    if (!usage) {
        return std::unexpected(CpuMonitorError::CalculationFailed);
    }

    // Step 6: Return the calculated percentage.
    return std::optional<double>{*usage};
}

}  // namespace pulse
