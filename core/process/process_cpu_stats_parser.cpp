#include "core/process/process_cpu_stats_parser.hpp"

#include <array>
#include <charconv>
#include <string_view>
#include <system_error>

namespace pulse {
namespace {

constexpr std::string_view whitespace = " \t\n\r\f\v";

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(whitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    return value.substr(first, value.find_last_not_of(whitespace) - first + 1);
}

template <typename T>
bool parseNumber(std::string_view value, T& result) {
    if (value.empty()) {
        return false;
    }
    const char* const begin = value.data();
    // from_chars needs the one-past-end pointer for this bounded string view.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char* const end = begin + value.size();
    const auto [ptr, ec] = std::from_chars(begin, end, result);
    return ec == std::errc{} && ptr == end;
}

}  // namespace

std::expected<ProcessCpuStats, ProcessCpuParseError> ProcessCpuStatsParser::parse(
    std::string_view content) {
    content = trim(content);
    if (content.empty()) {
        return std::unexpected(ProcessCpuParseError::MissingField);
    }

    const auto opening = content.find('(');
    if (opening == std::string_view::npos) {
        return std::unexpected(ProcessCpuParseError::InvalidField);
    }

    const auto pid = trim(content.substr(0, opening));
    if (pid.empty()) {
        return std::unexpected(ProcessCpuParseError::MissingField);
    }

    ProcessCpuStats result{};
    if (!parseNumber(pid, result.pid) || result.pid <= 0 ||
        whitespace.find(content[opening - 1]) == std::string_view::npos) {
        return std::unexpected(ProcessCpuParseError::InvalidField);
    }

    // The process name may itself contain spaces and parentheses.
    const auto closing = content.rfind(')');
    if (closing == std::string_view::npos || closing < opening) {
        return std::unexpected(ProcessCpuParseError::InvalidField);
    }

    auto remaining = content.substr(closing + 1);
    if (!remaining.empty() && whitespace.find(remaining.front()) == std::string_view::npos) {
        return std::unexpected(ProcessCpuParseError::InvalidField);
    }

    // Field 3 (state) is the first token after the name; retain fields through 22.
    constexpr std::size_t first_field = 3;
    constexpr std::size_t user_field = 14;
    constexpr std::size_t system_field = 15;
    constexpr std::size_t start_time_field = 22;
    std::array<std::string_view, start_time_field - first_field + 1> fields{};
    for (auto& field : fields) {
        remaining = trim(remaining);
        if (remaining.empty()) {
            return std::unexpected(ProcessCpuParseError::MissingField);
        }
        const auto end = remaining.find_first_of(whitespace);
        field = remaining.substr(0, end);
        remaining.remove_prefix(field.size());
    }

    if (!parseNumber(fields[user_field - first_field], result.user_ticks) ||
        !parseNumber(fields[system_field - first_field], result.system_ticks) ||
        !parseNumber(fields[start_time_field - first_field], result.start_time_ticks)) {
        return std::unexpected(ProcessCpuParseError::InvalidField);
    }

    return result;
}

}  // namespace pulse
