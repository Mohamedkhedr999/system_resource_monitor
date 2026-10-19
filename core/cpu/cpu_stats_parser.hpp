#ifndef SYSTEM_RESOURCE_MONITOR_CORE_CPU_STATS_PARSER_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_CPU_STATS_PARSER_HPP

#include <cstdint>
#include <expected>
#include <string_view>

#include "core/cpu/cpu_stats.hpp"

namespace pulse {

enum class CpuParseError : std::uint8_t {
    InvalidLabel,
    MissingFields,
    InvalidNumber,
    ExtraFields,
};

class CpuStatsParser {
   public:
    [[nodiscard]]
    static std::expected<CpuStats, CpuParseError> parse(std::string_view line);
};

}  // namespace pulse

#endif