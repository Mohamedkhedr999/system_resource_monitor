#include "core/process/process_stats_parser.hpp"

#include <charconv>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

namespace pulse {

namespace {

std::string_view trim(std::string_view value) {
    constexpr std::string_view whitespace = " \t\r";
    const auto first = value.find_first_not_of(whitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    return value.substr(first, value.find_last_not_of(whitespace) - first + 1);
}

template <typename T>
std::optional<T> parseUnsigned(std::string_view value) {
    T result{};

    const char* const begin = value.data();
    const char* const end = std::to_address(value.end());

    const auto [ptr, ec] = std::from_chars(begin, end, result);

    if (ec != std::errc{} || ptr != end) {
        return std::nullopt;
    }

    return result;
}

}  // namespace

std::expected<ProcessInfo, ProcessParseError> ProcessStatsParser::parse(std::string_view content) {
    std::istringstream stream{std::string{content}};
    std::string line;

    ProcessInfo result{};

    bool has_name = false;
    bool has_state = false;
    bool has_pid = false;

    while (std::getline(stream, line)) {
        std::istringstream line_stream{line};

        std::string key;
        line_stream >> key;

        if (key == "Name:") {
            if (has_name) {
                return std::unexpected(ProcessParseError::DuplicateField);
            }

            std::string name;
            std::getline(line_stream, name);
            const auto trimmed_name = trim(name);
            if (trimmed_name.empty()) {
                return std::unexpected(ProcessParseError::InvalidField);
            }

            result.name = trimmed_name;
            has_name = true;
        } else if (key == "State:") {
            if (has_state) {
                return std::unexpected(ProcessParseError::DuplicateField);
            }

            std::string state;
            if (!(line_stream >> state)) {
                return std::unexpected(ProcessParseError::InvalidField);
            }

            constexpr std::string_view valid_states = "RSDZTtXI";
            if (state.size() != 1 || valid_states.find(state.front()) == std::string_view::npos) {
                return std::unexpected(ProcessParseError::InvalidField);
            }

            std::string description;
            std::getline(line_stream, description);
            const auto trimmed_description = trim(description);
            if (!trimmed_description.empty() &&
                (trimmed_description.size() < 3 || trimmed_description.front() != '(' ||
                 trimmed_description.back() != ')')) {
                return std::unexpected(ProcessParseError::InvalidField);
            }

            result.state = state.front();
            has_state = true;
        } else if (key == "Pid:") {
            if (has_pid) {
                return std::unexpected(ProcessParseError::DuplicateField);
            }

            std::string value;
            std::string extra;
            if (!(line_stream >> value) || (line_stream >> extra)) {
                return std::unexpected(ProcessParseError::InvalidField);
            }

            const auto pid = parseUnsigned<std::uint32_t>(value);

            if (!pid || *pid == 0 ||
                *pid > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())) {
                return std::unexpected(ProcessParseError::InvalidField);
            }

            result.pid = static_cast<std::int32_t>(*pid);
            has_pid = true;
        } else if (key == "VmRSS:") {
            if (result.memory_kb.has_value()) {
                return std::unexpected(ProcessParseError::DuplicateField);
            }

            std::string value;
            std::string unit;
            std::string extra;

            if (!(line_stream >> value >> unit) || unit != "kB" || (line_stream >> extra)) {
                return std::unexpected(ProcessParseError::InvalidField);
            }

            const auto memory = parseUnsigned<std::uint64_t>(value);

            if (!memory) {
                return std::unexpected(ProcessParseError::InvalidField);
            }

            result.memory_kb = memory;
        }
    }

    if (!has_name || !has_state || !has_pid) {
        return std::unexpected(ProcessParseError::MissingRequiredField);
    }

    return result;
}

}  // namespace pulse
