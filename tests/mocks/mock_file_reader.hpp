#ifndef SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_FILE_READER_HPP
#define SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_FILE_READER_HPP

#include "core/io/interfaces/file_reader.hpp"
#include "gmock/gmock.h"

namespace pulse {

class MockFileReader : public IFileReader {
   public:
    MOCK_METHOD((std::expected<std::string, std::error_code>), read,
                (const std::filesystem::path& path), (const, override));
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_FILE_READER_HPP
