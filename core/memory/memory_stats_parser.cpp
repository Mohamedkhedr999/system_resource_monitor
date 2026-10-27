#include "core/memory/memory_stats_parser.hpp"

#include <cstdint>
#include <sstream>
#include <string>

namespace pulse {

std::expected<MemoryStats, MemoryParseError>
MemoryStatsParser::parse(std::string_view content) {
    std::istringstream input{std::string{content}};

    MemoryStats stats{};

    bool found_total = false;
    bool found_available = false;

    std::string line;

    while (std::getline(input, line)) {
        std::istringstream line_stream{line};

        std::string key;
        std::string value;
        std::string unit;
        std::string extra;

        if (!(line_stream >> key)) {
            continue;
        }

        // Ignore unrelated fields.
        if (key != "MemTotal:" && key != "MemAvailable:") {
            continue;
        }

        if (!(line_stream >> value >> unit) ||
            (line_stream >> extra) ||
            unit != "kB") {
            return std::unexpected(MemoryParseError::InvalidFormat);
        }

        // Reject negative numbers and nonnumeric values.
        if (value.empty() ||
            value.front() == '-' ||
            value.find_first_not_of("0123456789") != std::string::npos) {
            return std::unexpected(MemoryParseError::InvalidValue);
        }

        std::uint64_t parsed_value{};

        std::istringstream value_stream{value};

        if (!(value_stream >> parsed_value)) {
            return std::unexpected(MemoryParseError::InvalidValue);
        }

        if (key == "MemTotal:") {
            if (found_total) {
                return std::unexpected(MemoryParseError::InvalidFormat);
            }

            stats.total_kb = parsed_value;
            found_total = true;
        } else {
            if (found_available) {
                return std::unexpected(MemoryParseError::InvalidFormat);
            }

            stats.available_kb = parsed_value;
            found_available = true;
        }
    }

    if (!found_total || !found_available) {
        return std::unexpected(MemoryParseError::MissingField);
    }

    if (stats.total_kb == 0 ||
        stats.available_kb > stats.total_kb) {
        return std::unexpected(MemoryParseError::InvalidValue);
    }

    return stats;
}

}  // namespace pulse