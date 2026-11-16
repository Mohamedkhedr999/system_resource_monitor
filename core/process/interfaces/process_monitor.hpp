#ifndef SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_INTERFACES_PROCESS_MONITOR_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_INTERFACES_PROCESS_MONITOR_HPP

#include <expected>
#include <vector>

#include "core/process/interfaces/process_reader.hpp"
#include "core/process/process_info.hpp"

namespace pulse {

class IProcessMonitor {
   public:
    IProcessMonitor() = default;
    virtual ~IProcessMonitor() = default;

    IProcessMonitor(const IProcessMonitor&) = delete;
    IProcessMonitor& operator=(const IProcessMonitor&) = delete;
    IProcessMonitor(IProcessMonitor&&) = delete;
    IProcessMonitor& operator=(IProcessMonitor&&) = delete;

    [[nodiscard]]
    virtual std::expected<std::vector<ProcessInfo>, ProcessReadError> sample() = 0;
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_INTERFACES_PROCESS_MONITOR_HPP
