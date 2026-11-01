#ifndef SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_INFO_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_INFO_HPP

#include <cstdint>
#include <optional>
#include <string>

namespace pulse {

struct ProcessInfo {
    std::int32_t pid{};
    std::string name;
    char state{};
    std::optional<std::uint64_t> memory_kb;
};

}  // namespace pulse

#endif
