#include "core/memory/memory_monitor.hpp"

#include <stdexcept>
#include <utility>

namespace pulse {

MemoryMonitor::MemoryMonitor(std::unique_ptr<IMemoryReader> reader) : reader_(std::move(reader)) {
    if (!reader_) {
        throw std::invalid_argument("MemoryMonitor requires a valid IMemoryReader");
    }
}

std::expected<MemoryUsage, MemoryMonitorError> MemoryMonitor::sample() {
    const auto stats = reader_->read();
    if (!stats.has_value()) {
        return std::unexpected(MemoryMonitorError::ReadFailed);
    }

    if (stats->total_kb == 0 || stats->available_kb > stats->total_kb) {
        return std::unexpected(MemoryMonitorError::InvalidStats);
    }

    const auto used_kb = stats->total_kb - stats->available_kb;
    const auto percentage =
        (static_cast<double>(used_kb) / static_cast<double>(stats->total_kb)) * 100.0;

    return MemoryUsage{
        .used_kb = used_kb,
        .total_kb = stats->total_kb,
        .percentage = percentage,
    };
}

}  // namespace pulse
