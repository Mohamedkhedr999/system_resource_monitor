#ifndef SYSTEM_RESOURCE_MONITOR_CORE_CPU_CPU_MONITOR_IMPL_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_CPU_CPU_MONITOR_IMPL_HPP

#include <expected>
#include <memory>
#include <optional>

#include "core/cpu/interfaces/cpu_monitor.hpp"
#include "core/cpu/interfaces/cpu_reader.hpp"

namespace pulse {

class CpuMonitorImpl final : public ICpuMonitor {
   public:
    explicit CpuMonitorImpl(std::shared_ptr<ICpuReader> reader);

    [[nodiscard]]
    std::expected<std::optional<double>, CpuMonitorError> sample() override;

   private:
    std::shared_ptr<ICpuReader> reader_;
    std::optional<CpuStats> previous_;
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_CPU_CPU_MONITOR_IMPL_HPP
