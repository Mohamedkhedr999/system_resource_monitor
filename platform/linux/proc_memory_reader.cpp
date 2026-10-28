#include "platform/linux/proc_memory_reader.hpp"

#include <stdexcept>
#include <utility>

#include "core/memory/memory_stats_parser.hpp"

namespace pulse {

ProcMemoryReader::ProcMemoryReader(std::shared_ptr<IFileReader> file_reader, std::string path)
    : file_reader_(std::move(file_reader)), path_(std::move(path)) {
    if (!file_reader_) {
        throw std::invalid_argument("ProcMemoryReader requires a valid IFileReader");
    }
}

std::expected<MemoryStats, MemoryReadError> ProcMemoryReader::read() {
    const auto result = file_reader_->read(path_);
    if (!result.has_value()) {
        return std::unexpected(MemoryReadError::FileReadFailed);
    }

    const auto parse_result = MemoryStatsParser::parse(result.value());
    if (!parse_result.has_value()) {
        return std::unexpected(MemoryReadError::InvalidData);
    }

    return parse_result.value();
}

}  // namespace pulse
