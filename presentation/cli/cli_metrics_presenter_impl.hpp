#ifndef SYSTEM_RESOURCE_MONITOR_PRESENTATION_CLI_CLI_METRICS_PRESENTER_IMPL_HPP
#define SYSTEM_RESOURCE_MONITOR_PRESENTATION_CLI_CLI_METRICS_PRESENTER_IMPL_HPP

#include <iosfwd>

#include "core/monitoring/interfaces/metrics_presenter.hpp"

namespace pulse {

class CliMetricsPresenterImpl final : public IMetricsPresenter {
   public:
    explicit CliMetricsPresenterImpl(std::ostream& output);

    void presentCpuUsage(double percentage) override;
    void presentMemoryUsage(const MemoryUsage& usage) override;

   private:
    std::ostream& output_;
};

}  // namespace pulse

#endif
