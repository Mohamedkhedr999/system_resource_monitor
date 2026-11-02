#ifndef SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_PROCESS_READER_HPP
#define SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_PROCESS_READER_HPP

#include "core/process/interfaces/process_reader.hpp"
#include "gmock/gmock.h"

namespace pulse {

class MockProcessReader : public IProcessReader {
   public:
    MOCK_METHOD((std::expected<std::vector<ProcessInfo>, ProcessReadError>), read, (),
                (const, override));
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_PROCESS_READER_HPP
