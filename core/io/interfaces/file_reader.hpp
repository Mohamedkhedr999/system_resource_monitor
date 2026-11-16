#ifndef SYSTEM_RESOURCE_MONITOR_CORE_IO_INTERFACES_FILE_READER_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_IO_INTERFACES_FILE_READER_HPP

#include <expected>
#include <filesystem>
#include <string>
#include <system_error>

namespace pulse {

class IFileReader {
   public:
    IFileReader() = default;
    virtual ~IFileReader() = default;

    IFileReader(const IFileReader&) = delete;
    IFileReader& operator=(const IFileReader&) = delete;
    IFileReader(IFileReader&&) = delete;
    IFileReader& operator=(IFileReader&&) = delete;

    [[nodiscard]]
    virtual std::expected<std::string, std::error_code> read(
        const std::filesystem::path& path) const = 0;
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_IO_INTERFACES_FILE_READER_HPP
