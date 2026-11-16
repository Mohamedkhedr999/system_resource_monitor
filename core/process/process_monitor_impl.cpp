#include "core/process/process_monitor_impl.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>

#include "core/process/process_cpu_usage_calculator.hpp"

namespace pulse {

ProcessMonitorImpl::ProcessMonitorImpl(std::unique_ptr<IProcessReader> reader,
                                       std::uint64_t clock_ticks_per_second, Now now,
                                       std::size_t max_processes)
    : reader_(std::move(reader)),
      max_processes_(max_processes),
      clock_ticks_per_second_(clock_ticks_per_second),
      now_(std::move(now)) {
    if (!reader_) {
        throw std::invalid_argument("ProcessMonitorImpl requires a valid IProcessReader");
    }
    if (clock_ticks_per_second_ == 0) {
        throw std::invalid_argument("ProcessMonitorImpl requires a positive clock tick rate");
    }
    if (!now_) {
        throw std::invalid_argument("ProcessMonitorImpl requires a valid clock callable");
    }
}

std::expected<std::vector<ProcessInfo>, ProcessReadError> ProcessMonitorImpl::sample() {
    auto snapshots = reader_->read();
    if (!snapshots) {
        return std::unexpected(snapshots.error());
    }

    const auto sample_time = now_();
    std::unordered_map<std::int32_t, ProcessCpuStats> current_cpu_stats;
    current_cpu_stats.reserve(snapshots->size());
    std::vector<ProcessInfo> processes;
    processes.reserve(snapshots->size());

    for (auto& snapshot : *snapshots) {
        auto& info = snapshot.info;
        info.cpu_percentage.reset();
        const auto previous = previous_cpu_stats_.find(info.pid);
        if (has_previous_sample_ && previous != previous_cpu_stats_.end()) {
            const auto usage = ProcessCpuUsageCalculator::calculate(
                previous->second, snapshot.cpu_stats, sample_time - previous_sample_time_,
                clock_ticks_per_second_);
            if (usage) {
                info.cpu_percentage = *usage;
            }
        }
        current_cpu_stats.insert_or_assign(info.pid, snapshot.cpu_stats);
        processes.push_back(std::move(info));
    }

    // Retain all baselines, including processes outside the displayed list.
    previous_cpu_stats_ = std::move(current_cpu_stats);
    previous_sample_time_ = sample_time;
    has_previous_sample_ = true;

    // Optional comparison places unknown memory usage after every known value.
    std::ranges::stable_sort(processes, [](const ProcessInfo& lhs, const ProcessInfo& rhs) {
        return lhs.memory_kb > rhs.memory_kb;
    });

    if (processes.size() > max_processes_) {
        processes.resize(max_processes_);
    }

    return processes;
}

}  // namespace pulse
