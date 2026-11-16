#include "core/cpu/cpu_stats_parser.hpp"

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>

namespace pulse {

std::expected<CpuStats, CpuParseError> CpuStatsParser::parse(std::string_view line) {
    constexpr std::size_t kRequiredCounterCount = 8;
    constexpr std::size_t kUserIndex = 0;
    constexpr std::size_t kNiceIndex = 1;
    constexpr std::size_t kSystemIndex = 2;
    constexpr std::size_t kIdleIndex = 3;
    constexpr std::size_t kIoWaitIndex = 4;
    constexpr std::size_t kIrqIndex = 5;
    constexpr std::size_t kSoftIrqIndex = 6;
    constexpr std::size_t kStealIndex = 7;

    std::istringstream stream{std::string{line}};

    // Step 1: Validate the CPU label.
    std::string label;

    if (!(stream >> label) || label != "cpu") {
        return std::unexpected(CpuParseError::InvalidLabel);
    }

    // Step 2: Parse the eight required CPU counters.
    std::array<std::uint64_t, kRequiredCounterCount> values{};

    for (auto& value : values) {
        std::string token;

        if (!(stream >> token)) {
            return std::unexpected(CpuParseError::MissingFields);
        }

        const auto [ptr, ec] =
            std::from_chars(std::to_address(token.begin()), std::to_address(token.end()), value);

        if (ec != std::errc{} || ptr != std::to_address(token.end())) {
            return std::unexpected(CpuParseError::InvalidNumber);
        }
    }

    // Step 3: Validate optional guest and guest_nice counters.
    // They are not stored because Linux already includes them
    // in the user and nice counters.
    for (int i = 0; i < 2; ++i) {
        std::string token;

        if (!(stream >> token)) {
            break;
        }

        std::uint64_t ignored_value{0};

        const auto [ptr, ec] = std::from_chars(std::to_address(token.begin()),
                                               std::to_address(token.end()), ignored_value);

        if (ec != std::errc{} || ptr != std::to_address(token.end())) {
            return std::unexpected(CpuParseError::InvalidNumber);
        }
    }

    // Step 4: Reject unexpected additional fields.
    std::string extra_token;

    if (stream >> extra_token) {
        return std::unexpected(CpuParseError::ExtraFields);
    }

    // Step 5: Construct the result.
    return CpuStats{
        .user = values[kUserIndex],
        .nice = values[kNiceIndex],
        .system = values[kSystemIndex],
        .idle = values[kIdleIndex],
        .iowait = values[kIoWaitIndex],
        .irq = values[kIrqIndex],
        .softirq = values[kSoftIrqIndex],
        .steal = values[kStealIndex],
    };
}

}  // namespace pulse
