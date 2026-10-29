#ifndef SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_MEMORY_MONITOR_HPP
#define SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_MEMORY_MONITOR_HPP

#include "core/memory/interfaces/memory_monitor.hpp"
#include "gmock/gmock.h"

namespace pulse {

class MockMemoryMonitor : public IMemoryMonitor {
   public:
    MOCK_METHOD((std::expected<MemoryUsage, MemoryMonitorError>), sample, (), (override));
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_MEMORY_MONITOR_HPP
