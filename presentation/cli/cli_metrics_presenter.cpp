#include "presentation/cli/cli_metrics_presenter.hpp"

#include <ostream>

namespace pulse {

CliMetricsPresenter::CliMetricsPresenter(std::ostream& output)
    : IMetricsPresenter(), output_(output) {}

void CliMetricsPresenter::presentCpuUsage(double percentage) {
    output_ << "CPU Usage: " << percentage << "%\n";
}

}  // namespace pulse