#include "app/application_factory.hpp"

#include <iostream>
#include <memory>
#include <utility>

#include "core/cpu/cpu_monitor.hpp"
#include "platform/linux/file_reader_impl.hpp"
#include "platform/linux/proc_cpu_reader.hpp"
#include "presentation/cli/cli_metrics_presenter.hpp"

namespace pulse {

std::unique_ptr<MonitoringService> ApplicationFactory::createMonitoringService() {
    auto file_reader = std::make_shared<FileReaderImpl>();
    auto cpu_reader = std::make_shared<ProcCpuReader>(std::move(file_reader));
    auto cpu_monitor = std::make_unique<CpuMonitor>(std::move(cpu_reader));
    auto presenter = std::make_unique<CliMetricsPresenter>(std::cout);

    return std::make_unique<MonitoringService>(std::move(cpu_monitor), std::move(presenter));
}

}  // namespace pulse
