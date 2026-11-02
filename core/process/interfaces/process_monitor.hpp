#ifndef SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_INTERFACES_PROCESS_MONITOR_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_INTERFACES_PROCESS_MONITOR_HPP

#include <expected>
#include <vector>

#include "core/process/interfaces/process_reader.hpp"
#include "core/process/process_info.hpp"

namespace pulse {

class IProcessMonitor {
   public:
    virtual ~IProcessMonitor() = default;

    [[nodiscard]]
    virtual std::expected<std::vector<ProcessInfo>, ProcessReadError> sample() = 0;
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_INTERFACES_PROCESS_MONITOR_HPP
