#include "app/application_factory.hpp"

#include <iostream>
#include <memory>
#include <utility>

#include "core/cpu/cpu_monitor_impl.hpp"
#include "core/memory/memory_monitor_impl.hpp"
#include "core/monitoring/monitoring_service_impl.hpp"
#include "platform/linux/file_reader_impl.hpp"
#include "platform/linux/proc_cpu_reader_impl.hpp"
#include "platform/linux/proc_memory_reader_impl.hpp"
#include "presentation/cli/cli_metrics_presenter_impl.hpp"

namespace pulse {

std::unique_ptr<IMonitoringService> ApplicationFactory::createMonitoringService() {
    auto file_reader = std::make_shared<FileReaderImpl>();
    auto cpu_reader = std::make_shared<ProcCpuReaderImpl>(file_reader);
    auto cpu_monitor = std::make_unique<CpuMonitorImpl>(std::move(cpu_reader));
    auto memory_reader = std::make_unique<ProcMemoryReaderImpl>(file_reader);
    auto memory_monitor = std::make_unique<MemoryMonitorImpl>(std::move(memory_reader));
    auto presenter = std::make_unique<CliMetricsPresenterImpl>(std::cout);

    return std::make_unique<MonitoringServiceImpl>(std::move(cpu_monitor),
                                                   std::move(memory_monitor), std::move(presenter));
}

}  // namespace pulse
