#pragma once

#include "core/process/process_cpu_stats.hpp"
#include "core/process/process_info.hpp"

namespace pulse {

struct ProcessSnapshot {
    ProcessInfo info;
    ProcessCpuStats cpu_stats;
};

}  // namespace pulse
