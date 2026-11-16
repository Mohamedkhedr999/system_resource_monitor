#include "platform/linux/proc_memory_reader_impl.hpp"

#include <array>
#include <expected>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "tests/mocks/mock_file_reader.hpp"

namespace pulse {
namespace {

using FileReadResult = std::expected<std::string, std::error_code>;

class ProcMemoryReaderImplTest : public testing::Test {
   protected:
    std::shared_ptr<testing::StrictMock<MockFileReader>> file_reader_ =
        std::make_shared<testing::StrictMock<MockFileReader>>();
};

TEST_F(ProcMemoryReaderImplTest, ReadsDefaultPathAndParsesFieldsAcrossTheWholeFile) {
    EXPECT_CALL(*file_reader_, read(std::filesystem::path("/proc/meminfo")))
        .WillOnce(testing::Return(
            FileReadResult("MemTotal: 16000 kB\nMemFree: 1000 kB\nMemAvailable: 10000 kB\n")));
    ProcMemoryReaderImpl reader(file_reader_);

    const auto result = reader.read();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->total_kb, 16000U);
    EXPECT_EQ(result->available_kb, 10000U);
}

TEST_F(ProcMemoryReaderImplTest, ForwardsCustomPath) {
    const std::string path = "/test/custom-meminfo";
    EXPECT_CALL(*file_reader_, read(std::filesystem::path(path)))
        .WillOnce(testing::Return(FileReadResult("MemTotal: 100 kB\nMemAvailable: 40 kB")));
    ProcMemoryReaderImpl reader(file_reader_, path);

    const auto result = reader.read();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->total_kb, 100U);
    EXPECT_EQ(result->available_kb, 40U);
}

TEST_F(ProcMemoryReaderImplTest, MapsFileErrorsToFileReadFailed) {
    ProcMemoryReaderImpl reader(file_reader_);
    for (const auto error : std::array{std::errc::no_such_file_or_directory,
                                       std::errc::permission_denied, std::errc::io_error}) {
        SCOPED_TRACE(std::make_error_code(error).message());
        EXPECT_CALL(*file_reader_, read(std::filesystem::path("/proc/meminfo")))
            .WillOnce(
                testing::Return(FileReadResult(std::unexpected(std::make_error_code(error)))));

        const auto result = reader.read();

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), MemoryReadError::FileReadFailed);
    }
}

TEST_F(ProcMemoryReaderImplTest, MapsMissingMalformedAndInvalidDataToInvalidData) {
    ProcMemoryReaderImpl reader(file_reader_);
    for (const std::string_view content :
         std::array{"", "MemTotal: 100 kB\n", "MemTotal: 100 MB\nMemAvailable: 40 kB\n",
                    "MemTotal: abc kB\nMemAvailable: 40 kB\n",
                    "MemTotal: 100 kB\nMemAvailable: 101 kB\n"}) {
        SCOPED_TRACE(content);
        EXPECT_CALL(*file_reader_, read(std::filesystem::path("/proc/meminfo")))
            .WillOnce(testing::Return(FileReadResult(std::string{content})));

        const auto result = reader.read();

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), MemoryReadError::InvalidData);
    }
}

TEST_F(ProcMemoryReaderImplTest, CollectsFreshStatsOnEachRead) {
    EXPECT_CALL(*file_reader_, read(std::filesystem::path("/proc/meminfo")))
        .WillOnce(testing::Return(FileReadResult("MemTotal: 100 kB\nMemAvailable: 40 kB\n")))
        .WillOnce(testing::Return(FileReadResult("MemTotal: 100 kB\nMemAvailable: 25 kB\n")));
    ProcMemoryReaderImpl reader(file_reader_);

    const auto first = reader.read();
    const auto second = reader.read();

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first->total_kb, 100U);
    EXPECT_EQ(first->available_kb, 40U);
    EXPECT_EQ(second->total_kb, 100U);
    EXPECT_EQ(second->available_kb, 25U);
}

TEST(ProcMemoryReaderConstructorTest, RejectsNullFileReader) {
    EXPECT_THROW((void)ProcMemoryReaderImpl(nullptr), std::invalid_argument);
}

}  // namespace
}  // namespace pulse
