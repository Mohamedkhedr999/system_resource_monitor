#ifndef SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_INTERFACES_METRICS_PRESENTER_HPP
#define SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_INTERFACES_METRICS_PRESENTER_HPP

namespace pulse {

class IMetricsPresenter {
   public:
    virtual ~IMetricsPresenter() = default;

    virtual void presentCpuUsage(double percentage) = 0;
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_CORE_MONITORING_INTERFACES_METRICS_PRESENTER_HPP
