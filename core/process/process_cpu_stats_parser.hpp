#ifndef SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_CPU_STATS_PARSER_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_CPU_STATS_PARSER_HPP

#include <expected>
#include <string_view>

#include "core/process/process_cpu_stats.hpp"

namespace pulse {

enum class ProcessCpuParseError { MissingField, InvalidField };

class ProcessCpuStatsParser {
   public:
    [[nodiscard]]
    static std::expected<ProcessCpuStats, ProcessCpuParseError> parse(std::string_view content);
};

}  // namespace pulse

#endif
