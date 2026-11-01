#pragma once

#include <expected>
#include <vector>

#include "core/process/process_info.hpp"

namespace pulse {

enum class ProcessReadError : std::uint8_t { DirectoryReadFailed };

class IProcessReader {
   public:
    virtual ~IProcessReader() = default;

    [[nodiscard]]
    virtual std::expected<std::vector<ProcessInfo>, ProcessReadError> read() const = 0;
};

}  // namespace pulse
