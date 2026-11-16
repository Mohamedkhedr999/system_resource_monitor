#ifndef SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_MEMORY_READER_HPP
#define SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_MEMORY_READER_HPP

#include "core/memory/interfaces/memory_reader.hpp"
#include "gmock/gmock.h"

namespace pulse {

class MockMemoryReader : public IMemoryReader {
   public:
    MOCK_METHOD((std::expected<MemoryStats, MemoryReadError>), read, (), (override));
};

}  // namespace pulse

#endif
