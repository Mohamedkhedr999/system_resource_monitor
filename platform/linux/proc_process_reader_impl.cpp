
#include "platform/linux/proc_process_reader_impl.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#include "core/process/process_cpu_stats_parser.hpp"
#include "core/process/process_stats_parser.hpp"

namespace pulse {

namespace {

bool isNumericPid(const std::string& name) {
    return !name.empty() &&
           std::ranges::all_of(name, [](unsigned char ch) { return ch >= '0' && ch <= '9'; });
}

}  // namespace

ProcProcessReader::ProcProcessReader(std::shared_ptr<IFileReader> file_reader,
                                     std::filesystem::path proc_path)
    : file_reader_(std::move(file_reader)), proc_path_(std::move(proc_path)) {
    if (!file_reader_) {
        throw std::invalid_argument("ProcProcessReader requires a valid file reader");
    }
}

std::expected<std::vector<ProcessSnapshot>, ProcessReadError> ProcProcessReader::read() const {
    namespace fs = std::filesystem;

    std::vector<ProcessSnapshot> processes;
    std::error_code ec;

    fs::directory_iterator iterator{proc_path_, ec};

    if (ec) {
        return std::unexpected(ProcessReadError::DirectoryReadFailed);
    }

    const fs::directory_iterator end{};

    while (iterator != end) {
        const auto& entry = *iterator;
        const auto name = entry.path().filename().string();

        if (isNumericPid(name)) {
            const auto status_content = file_reader_->read(entry.path() / "status");
            const auto stat_content = file_reader_->read(entry.path() / "stat");

            if (status_content && stat_content) {
                const auto process = ProcessStatsParser::parse(*status_content);
                const auto cpu_stats = ProcessCpuStatsParser::parse(*stat_content);

                if (process && cpu_stats && process->pid == cpu_stats->pid) {
                    processes.push_back(ProcessSnapshot{.info = *process, .cpu_stats = *cpu_stats});
                }
            }
        }

        iterator.increment(ec);

        if (ec) {
            return std::unexpected(ProcessReadError::DirectoryReadFailed);
        }
    }

    return processes;
}

}  // namespace pulse
