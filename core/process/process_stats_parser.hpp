#ifndef SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_STATS_PARSER_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_PROCESS_PROCESS_STATS_PARSER_HPP

#include <cstdint>
#include <expected>
#include <string_view>

#include "core/process/process_info.hpp"

namespace pulse {

enum class ProcessParseError : std::uint8_t { MissingRequiredField, InvalidField, DuplicateField };

class ProcessStatsParser {
   public:
    [[nodiscard]]
    static std::expected<ProcessInfo, ProcessParseError> parse(std::string_view content);
};

}  // namespace pulse

#endif
