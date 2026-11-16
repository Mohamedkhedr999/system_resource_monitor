#ifndef SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_INTERFACES_MEMORY_READER_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_MEMORY_INTERFACES_MEMORY_READER_HPP

#include <expected>

#include "core/memory/memory_stats.hpp"

namespace pulse {

enum class MemoryReadError { FileReadFailed, InvalidData };

class IMemoryReader {
   public:
    IMemoryReader() = default;
    virtual ~IMemoryReader() = default;

    IMemoryReader(const IMemoryReader&) = delete;
    IMemoryReader& operator=(const IMemoryReader&) = delete;
    IMemoryReader(IMemoryReader&&) = delete;
    IMemoryReader& operator=(IMemoryReader&&) = delete;

    [[nodiscard]]
    virtual std::expected<MemoryStats, MemoryReadError> read() = 0;
};

}  // namespace pulse

#endif
