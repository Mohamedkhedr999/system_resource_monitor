#include "platform/linux/proc_cpu_reader_impl.hpp"

#include <filesystem>
#include <string_view>
#include <utility>

#include "core/cpu/cpu_stats_parser.hpp"

namespace pulse {

ProcCpuReaderImpl::ProcCpuReaderImpl(std::shared_ptr<IFileReader> file_reader,
                                     std::filesystem::path path)
    : ICpuReader(), file_reader_(std::move(file_reader)), path_(std::move(path)) {
    if (!file_reader_) {
        throw std::invalid_argument("ProcCpuReaderImpl requires a valid IFileReader");
    }
}

std::expected<CpuStats, CpuReadError> ProcCpuReaderImpl::read() const {
    const auto result = file_reader_->read(path_);
    if (!result.has_value()) {
        return std::unexpected(CpuReadError::FileReadFailed);
    }

    const std::string_view content{result.value()};
    const auto first_line = content.substr(0, content.find('\n'));

    const auto parse_result = CpuStatsParser::parse(first_line);

    if (!parse_result.has_value()) {
        return std::unexpected(CpuReadError::InvalidData);
    }

    return parse_result.value();
}

}  // namespace pulse
