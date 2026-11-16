#ifndef SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_MONITOR_IMPL_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_MONITOR_IMPL_HPP

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "core/process/interfaces/process_monitor.hpp"
#include "core/process/interfaces/process_reader.hpp"
#include "core/process/process_cpu_stats.hpp"
#include "core/process/process_info.hpp"

namespace pulse {

class ProcessMonitorImpl final : public IProcessMonitor {
   public:
    using Clock = std::chrono::steady_clock;
    using Now = std::function<Clock::time_point()>;

    explicit ProcessMonitorImpl(std::unique_ptr<IProcessReader> reader,
                                std::uint64_t clock_ticks_per_second, Now now = Clock::now,
                                std::size_t max_processes = 10);

    [[nodiscard]]
    std::expected<std::vector<ProcessInfo>, ProcessReadError> sample() override;

   private:
    std::unique_ptr<IProcessReader> reader_;
    std::size_t max_processes_;
    std::uint64_t clock_ticks_per_second_;
    Now now_;
    std::unordered_map<std::int32_t, ProcessCpuStats> previous_cpu_stats_;
    Clock::time_point previous_sample_time_{};
    bool has_previous_sample_{false};
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_MONITOR_IMPL_HPP
