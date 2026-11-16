#pragma once

#include <cstdint>
#include <expected>
#include <vector>

#include "core/process/process_snapshot.hpp"

namespace pulse {

enum class ProcessReadError : std::uint8_t { DirectoryReadFailed };

class IProcessReader {
   public:
    IProcessReader() = default;
    virtual ~IProcessReader() = default;

    IProcessReader(const IProcessReader&) = delete;
    IProcessReader& operator=(const IProcessReader&) = delete;
    IProcessReader(IProcessReader&&) = delete;
    IProcessReader& operator=(IProcessReader&&) = delete;

    [[nodiscard]]
    virtual std::expected<std::vector<ProcessSnapshot>, ProcessReadError> read() const = 0;
};

}  // namespace pulse
