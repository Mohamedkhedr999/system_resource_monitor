#ifndef SYSTEM_RESOURCE_MONITOR_APP_PULSE_APPLICATION_HPP
#define SYSTEM_RESOURCE_MONITOR_APP_PULSE_APPLICATION_HPP

#include <chrono>
#include <expected>
#include <memory>
#include <stop_token>

#include "core/cpu/interfaces/cpu_monitor.hpp"
#include "core/monitoring/interfaces/monitoring_service.hpp"

namespace pulse {

class PulseApplication {
   public:
    explicit PulseApplication(std::unique_ptr<IMonitoringService> monitoring_service,
                              std::chrono::milliseconds interval = std::chrono::seconds{1});

    // Blocks until stopped or sampling fails. Call from only one thread at a time.
    [[nodiscard]]
    std::expected<void, CpuMonitorError> run(const std::stop_token& stop_token);

   private:
    std::unique_ptr<IMonitoringService> monitoring_service_;
    std::chrono::milliseconds interval_;
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_APP_PULSE_APPLICATION_HPP
