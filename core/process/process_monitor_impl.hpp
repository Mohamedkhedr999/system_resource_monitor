#ifndef SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_MONITOR_IMPL_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_MONITOR_IMPL_HPP

#include <cstddef>
#include <expected>
#include <memory>
#include <vector>

#include "core/process/interfaces/process_monitor.hpp"
#include "core/process/interfaces/process_reader.hpp"
#include "core/process/process_info.hpp"

namespace pulse {

class ProcessMonitorImpl final : public IProcessMonitor {
   public:
    explicit ProcessMonitorImpl(std::unique_ptr<IProcessReader> reader,
                                std::size_t max_processes = 10);

    [[nodiscard]]
    std::expected<std::vector<ProcessInfo>, ProcessReadError> sample() override;

   private:
    std::unique_ptr<IProcessReader> reader_;
    std::size_t max_processes_;
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_MONITOR_IMPL_HPP
