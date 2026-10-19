#ifndef SYSTEM_RESOURCE_MONITOR_CORE_INTERFACES_CPU_READER_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_INTERFACES_CPU_READER_HPP

#include <expected>
#include <system_error>

#include "core/cpu_stats.hpp"

namespace pulse {

class ICpuReader {
   public:
    virtual ~ICpuReader() = default;

    [[nodiscard]]
    virtual std::expected<CpuStats, std::error_code> read() const = 0;
};

}  // namespace pulse

#endif
