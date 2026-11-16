#ifndef SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_INTERFACES_METRICS_PRESENTER_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_INTERFACES_METRICS_PRESENTER_HPP

#include <vector>

#include "core/memory/memory_usage.hpp"
#include "core/process/process_info.hpp"

namespace pulse {

class IMetricsPresenter {
   public:
    IMetricsPresenter() = default;
    virtual ~IMetricsPresenter() = default;

    IMetricsPresenter(const IMetricsPresenter&) = delete;
    IMetricsPresenter& operator=(const IMetricsPresenter&) = delete;
    IMetricsPresenter(IMetricsPresenter&&) = delete;
    IMetricsPresenter& operator=(IMetricsPresenter&&) = delete;

    // Starts one update before any of its metrics are presented.
    virtual void beginFrame() = 0;
    virtual void presentCpuUsage(double percentage) = 0;
    virtual void presentMemoryUsage(const MemoryUsage& usage) = 0;
    virtual void presentProcesses(const std::vector<ProcessInfo>& processes) = 0;
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_INTERFACES_METRICS_PRESENTER_HPP
