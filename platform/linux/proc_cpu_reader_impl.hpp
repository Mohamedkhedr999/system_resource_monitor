#ifndef SYSTEM_RESOURCE_MONITOR_PLATFORM_LINUX_PROC_CPU_READER_IMPL_HPP
#define SYSTEM_RESOURCE_MONITOR_PLATFORM_LINUX_PROC_CPU_READER_IMPL_HPP

#include <filesystem>
#include <memory>

#include "core/cpu/interfaces/cpu_reader.hpp"
#include "core/io/interfaces/file_reader.hpp"

namespace pulse {

class ProcCpuReaderImpl final : public ICpuReader {
   public:
    explicit ProcCpuReaderImpl(std::shared_ptr<IFileReader> file_reader,
                               std::filesystem::path path = "/proc/stat");

    [[nodiscard]]
    std::expected<CpuStats, CpuReadError> read() const override;

   private:
    std::shared_ptr<IFileReader> file_reader_;
    std::filesystem::path path_;
};

}  // namespace pulse

#endif
