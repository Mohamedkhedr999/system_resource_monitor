#ifndef SYSTEM_RESOURCE_MONITOR_CORE_CPU_INTERFACES_CPU_READER_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_CPU_INTERFACES_CPU_READER_HPP

#include <cstdint>
#include <expected>
#include <system_error>

#include "core/cpu/cpu_stats.hpp"

namespace pulse {

enum class CpuReadError : std::uint8_t { FileOpenFailed, FileReadFailed, InvalidData };

class ICpuReader {
   public:
    virtual ~ICpuReader() = default;

    [[nodiscard]]
    virtual std::expected<CpuStats, CpuReadError> read() const = 0;
};

}  // namespace pulse

#endif
