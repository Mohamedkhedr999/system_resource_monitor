#ifndef SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_CPU_MONITOR_HPP
#define SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_CPU_MONITOR_HPP

#include "core/cpu/interfaces/cpu_monitor.hpp"
#include "gmock/gmock.h"

namespace pulse {

class MockCpuMonitor : public ICpuMonitor {
   public:
    MOCK_METHOD((std::expected<std::optional<double>, CpuMonitorError>), sample, (), (override));
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_CPU_MONITOR_HPP
