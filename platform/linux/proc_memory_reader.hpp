#ifndef SYSTEM_RESOURCE_MONITOR_PLATFORM_LINUX_PROC_MEMORY_READER_HPP
#define SYSTEM_RESOURCE_MONITOR_PLATFORM_LINUX_PROC_MEMORY_READER_HPP

#include <expected>
#include <memory>
#include <string>

#include "core/io/interfaces/file_reader.hpp"
#include "core/memory/interfaces/memory_reader.hpp"

namespace pulse {

class ProcMemoryReader final : public IMemoryReader {
   public:
    explicit ProcMemoryReader(std::shared_ptr<IFileReader> file_reader,
                              std::string path = "/proc/meminfo");

    [[nodiscard]]
    std::expected<MemoryStats, MemoryReadError> read() override;

   private:
    std::shared_ptr<IFileReader> file_reader_;
    std::string path_;
};

}  // namespace pulse

#endif
