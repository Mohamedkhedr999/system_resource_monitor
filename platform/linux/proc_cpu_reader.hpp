#ifndef SYSTEM_RESOURCE_MONITOR_PLATFORM_LINUX_PROC_CPU_READER_HPP
#define SYSTEM_RESOURCE_MONITOR_PLATFORM_LINUX_PROC_CPU_READER_HPP

#include <filesystem>
#include "core/cpu/interfaces/cpu_reader.hpp"

namespace pulse {

class ProcCpuReader final : public ICpuReader {
public:
    explicit ProcCpuReader(
        std::filesystem::path path = "/proc/stat");

    [[nodiscard]]
    std::expected<CpuStats, CpuReadError> read() const override;

private:
    std::filesystem::path path_;
};

}  // namespace pulse

#endif
