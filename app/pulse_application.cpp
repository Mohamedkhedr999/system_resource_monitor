#include "app/pulse_application.hpp"

#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace pulse {

PulseApplication::PulseApplication(std::unique_ptr<IMonitoringService> monitoring_service,
                                   std::chrono::milliseconds interval)
    : monitoring_service_(std::move(monitoring_service)), interval_(interval) {
    if (!monitoring_service_) {
        throw std::invalid_argument("PulseApplication requires a monitoring service");
    }
    if (interval_ <= std::chrono::milliseconds::zero()) {
        throw std::invalid_argument("PulseApplication requires a positive monitoring interval");
    }
}

std::expected<void, CpuMonitorError> PulseApplication::run(const std::stop_token& stop_token) {
    std::mutex wait_mutex;
    std::condition_variable_any wake_up;
    std::unique_lock lock(wait_mutex);

    auto next_sample = std::chrono::steady_clock::now();
    while (!stop_token.stop_requested()) {
        const auto result = monitoring_service_->tick();

        if (!result) {
            return std::unexpected(result.error());
        }

        next_sample += interval_;

        // The stop-aware wait also wakes when cancellation is requested.
        wake_up.wait_until(lock, stop_token, next_sample, [] { return false; });
    }

    return {};
}

}  // namespace pulse
