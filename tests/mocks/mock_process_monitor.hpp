#ifndef SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_PROCESS_MONITOR_HPP
#define SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_PROCESS_MONITOR_HPP

#include "core/process/interfaces/process_monitor.hpp"
#include "gmock/gmock.h"

namespace pulse {

class MockProcessMonitor : public IProcessMonitor {
   public:
    MOCK_METHOD((std::expected<std::vector<ProcessInfo>, ProcessReadError>), sample, (),
                (override));
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_PROCESS_MONITOR_HPP
