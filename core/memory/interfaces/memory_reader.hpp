#ifndef SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_INTERFACES_MEMORY_READER_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_INTERFACES_MEMORY_READER_HPP

#include <expected>

#include "core/memory/memory_stats.hpp"

namespace pulse {

enum class MemoryReadError {
    FileReadFailed,
    InvalidData
};

class IMemoryReader {
public:
    virtual ~IMemoryReader() = default;

    [[nodiscard]]
    virtual std::expected<MemoryStats, MemoryReadError> read() = 0;
};

}  // namespace pulse

#endif
