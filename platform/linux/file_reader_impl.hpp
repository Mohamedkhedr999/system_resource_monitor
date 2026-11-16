#ifndef SYSTEM_RESOURCE_MONITOR_PLATFORM_LINUX_FILE_READER_IMPL_HPP
#define SYSTEM_RESOURCE_MONITOR_PLATFORM_LINUX_FILE_READER_IMPL_HPP

#include <expected>
#include <filesystem>
#include <string>
#include <system_error>

#include "core/io/interfaces/file_reader.hpp"

namespace pulse {

class FileReaderImpl final : public IFileReader {
   public:
    FileReaderImpl();

    [[nodiscard]]
    std::expected<std::string, std::error_code> read(
        const std::filesystem::path& path) const override;
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_PLATFORM_LINUX_FILE_READER_IMPL_HPP
