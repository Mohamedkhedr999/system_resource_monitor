#include "app/application_factory.hpp"

#include "gtest/gtest.h"

namespace pulse {
namespace {

TEST(ApplicationFactoryTest, CreatesMonitoringService) {
    const auto service = ApplicationFactory::createMonitoringService();

    ASSERT_NE(service, nullptr);
}

TEST(ApplicationFactoryTest, CreatesDistinctServicesForSeparateCalls) {
    const auto first = ApplicationFactory::createMonitoringService();
    const auto second = ApplicationFactory::createMonitoringService();

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_NE(first.get(), second.get());
}

}  // namespace
}  // namespace pulse
