#ifndef SYSTEM_RESOURCE_MONITOR_PRESENTATION_CLI_CLI_METRICS_PRESENTER_HPP
#define SYSTEM_RESOURCE_MONITOR_PRESENTATION_CLI_CLI_METRICS_PRESENTER_HPP

#include <iosfwd>

#include "core/monitoring/interfaces/metrics_presenter.hpp"

namespace pulse {

class CliMetricsPresenter final : public IMetricsPresenter {
public:
    explicit CliMetricsPresenter(std::ostream& output);

    void presentCpuUsage(double percentage) override;

private:
    std::ostream& output_;
};

}  // namespace pulse

#endif