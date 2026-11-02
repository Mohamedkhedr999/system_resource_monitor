#ifndef SYSTEM_RESOURCE_MONITOR_PRESENTATION_CLI_CLI_METRICS_PRESENTER_IMPL_HPP
#define SYSTEM_RESOURCE_MONITOR_PRESENTATION_CLI_CLI_METRICS_PRESENTER_IMPL_HPP

#include <iosfwd>
#include <vector>

#include "core/monitoring/interfaces/metrics_presenter.hpp"
#include "core/process/process_info.hpp"

namespace pulse {

class CliMetricsPresenterImpl final : public IMetricsPresenter {
   public:
    explicit CliMetricsPresenterImpl(std::ostream& output);

    void presentCpuUsage(double percentage) override;
    void presentMemoryUsage(const MemoryUsage& usage) override;
    void presentProcesses(const std::vector<ProcessInfo>& processes) override;

   private:
    std::ostream& output_;
};

}  // namespace pulse

#endif
