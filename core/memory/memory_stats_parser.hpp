#ifndef SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_MEMORY_STATS_PARSER_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_MEMORY_STATS_PARSER_HPP

#include <expected>
#include <string_view>

#include "core/memory/memory_stats.hpp"

namespace pulse {

enum class MemoryParseError : std::uint8_t { MissingField, InvalidFormat, InvalidValue };

class MemoryStatsParser {
   public:
    [[nodiscard]]
    static std::expected<MemoryStats, MemoryParseError> parse(std::string_view content);
};

}  // namespace pulse

#endif