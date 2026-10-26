#include "presentation/cli/cli_metrics_presenter.hpp"

#include <iomanip>
#include <ostream>

namespace pulse {

CliMetricsPresenter::CliMetricsPresenter(std::ostream& output)
    : IMetricsPresenter(), output_(output) {}

void CliMetricsPresenter::presentCpuUsage(double percentage) {
    output_ << std::fixed << std::setprecision(2) << "CPU Usage: " << percentage << "%\n";
}

}  // namespace pulse