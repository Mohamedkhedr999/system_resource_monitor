#include "core/process/process_monitor_impl.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace pulse {

ProcessMonitorImpl::ProcessMonitorImpl(std::unique_ptr<IProcessReader> reader,
                                       std::size_t max_processes)
    : reader_(std::move(reader)), max_processes_(max_processes) {
    if (!reader_) {
        throw std::invalid_argument("ProcessMonitorImpl requires a valid IProcessReader");
    }
}

std::expected<std::vector<ProcessInfo>, ProcessReadError> ProcessMonitorImpl::sample() {
    auto processes = reader_->read();
    if (!processes) {
        return std::unexpected(processes.error());
    }

    // Optional comparison places unknown memory usage after every known value.
    std::ranges::stable_sort(*processes, [](const ProcessInfo& lhs, const ProcessInfo& rhs) {
        return lhs.memory_kb > rhs.memory_kb;
    });

    if (processes->size() > max_processes_) {
        processes->resize(max_processes_);
    }

    return processes;
}

}  // namespace pulse
