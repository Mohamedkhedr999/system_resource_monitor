#include "platform/linux/file_reader_impl.hpp"

#include <cerrno>
#include <fstream>

namespace pulse {

FileReaderImpl::FileReaderImpl() : IFileReader() {}

std::expected<std::string, std::error_code> FileReaderImpl::read(
    const std::filesystem::path& path) const {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return std::unexpected(std::make_error_code(std::errc::io_error));
    }

    try {
        std::string content{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};

        if (file.bad()) {
            return std::unexpected(std::make_error_code(std::errc::io_error));
        }

        return content;

    } catch (const std::ios_base::failure&) {
        return std::unexpected(std::make_error_code(std::errc::io_error));
    }
}

}  // namespace pulse
