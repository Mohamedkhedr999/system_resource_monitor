#ifndef SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_CPU_READER_HPP
#define SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_CPU_READER_HPP

#include "core/cpu/interfaces/cpu_reader.hpp"
#include "gmock/gmock.h"

namespace pulse {

class MockCpuReader : public ICpuReader {
   public:
    MOCK_METHOD((std::expected<CpuStats, CpuReadError>), read, (), (const, override));
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_CPU_READER_HPP
