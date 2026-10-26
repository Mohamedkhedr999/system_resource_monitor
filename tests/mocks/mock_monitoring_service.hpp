#ifndef SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_MONITORING_SERVICE_HPP
#define SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_MONITORING_SERVICE_HPP

#include <expected>

#include "core/monitoring/interfaces/monitoring_service.hpp"
#include "gmock/gmock.h"

namespace pulse {

class MockMonitoringService : public IMonitoringService {
   public:
    MOCK_METHOD((std::expected<void, CpuMonitorError>), tick, (), (override));
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_MONITORING_SERVICE_HPP
