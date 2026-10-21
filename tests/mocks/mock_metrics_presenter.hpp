#ifndef SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_METRICS_PRESENTER_HPP
#define SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_METRICS_PRESENTER_HPP

#include "core/monitoring/interfaces/metrics_presenter.hpp"
#include "gmock/gmock.h"

namespace pulse {

class MockMetricsPresenter : public IMetricsPresenter {
   public:
    MOCK_METHOD(void, presentCpuUsage, (double percentage), (override));
};

}  // namespace pulse

#endif  // SYSTEM_RESOURCE_MONITOR_TESTS_MOCKS_MOCK_METRICS_PRESENTER_HPP
