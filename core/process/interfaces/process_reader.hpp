#pragma once

#include <cstdint>
#include <expected>
#include <vector>

#include "core/process/process_snapshot.hpp"

namespace pulse {

enum class ProcessReadError : std::uint8_t { DirectoryReadFailed };

class IProcessReader {
   public:
    virtual ~IProcessReader() = default;

    [[nodiscard]]
    virtual std::expected<std::vector<ProcessSnapshot>, ProcessReadError> read() const = 0;
};

}  // namespace pulse
