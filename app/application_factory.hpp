#ifndef SYSTEM_RESOURCE_MONITOR_APP_APPLICATION_FACTORY_HPP
#define SYSTEM_RESOURCE_MONITOR_APP_APPLICATION_FACTORY_HPP

#include <iosfwd>
#include <memory>

#include "core/monitoring/monitoring_service.hpp"

namespace pulse {

class ApplicationFactory {
   public:
    // The output stream must outlive the returned service.
    [[nodiscard]]
    static std::unique_ptr<MonitoringService> createMonitoringService();
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_APP_APPLICATION_FACTORY_HPP
