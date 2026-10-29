#include "platform/linux/proc_cpu_reader_impl.hpp"

#include <expected>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "tests/mocks/mock_file_reader.hpp"

namespace pulse {
namespace {

using FileReadResult = std::expected<std::string, std::error_code>;

class ProcCpuReaderImplTest : public testing::Test {
   protected:
    std::shared_ptr<testing::StrictMock<MockFileReader>> file_reader_ =
        std::make_shared<testing::StrictMock<MockFileReader>>();
};

TEST_F(ProcCpuReaderImplTest, ReadsDefaultPathAndParsesOnlyFirstLine) {
    EXPECT_CALL(*file_reader_, read(std::filesystem::path("/proc/stat")))
        .WillOnce(testing::Return(
            FileReadResult("cpu 11 22 33 44 55 66 77 88\ncpu0 1 2 3 4 5 6 7 8\nintr 123\n")));
    const ProcCpuReaderImpl reader(file_reader_);

    const auto result = reader.read();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->user, 11U);
    EXPECT_EQ(result->nice, 22U);
    EXPECT_EQ(result->system, 33U);
    EXPECT_EQ(result->idle, 44U);
    EXPECT_EQ(result->iowait, 55U);
    EXPECT_EQ(result->irq, 66U);
    EXPECT_EQ(result->softirq, 77U);
    EXPECT_EQ(result->steal, 88U);
}

TEST_F(ProcCpuReaderImplTest, ForwardsCustomPath) {
    const std::filesystem::path path("/test/custom-stat");
    EXPECT_CALL(*file_reader_, read(path))
        .WillOnce(testing::Return(FileReadResult("cpu 1 2 3 4 5 6 7 8\n")));
    const ProcCpuReaderImpl reader(file_reader_, path);

    const auto result = reader.read();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->user, 1U);
}

TEST_F(ProcCpuReaderImplTest, AcceptsFirstLineWithoutTrailingNewline) {
    EXPECT_CALL(*file_reader_, read(std::filesystem::path("/proc/stat")))
        .WillOnce(testing::Return(FileReadResult("cpu 1 2 3 4 5 6 7 8")));
    const ProcCpuReaderImpl reader(file_reader_);

    const auto result = reader.read();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->user, 1U);
    EXPECT_EQ(result->steal, 8U);
}

TEST_F(ProcCpuReaderImplTest, MapsFileErrorsToFileReadFailed) {
    const ProcCpuReaderImpl reader(file_reader_);
    for (const auto error : {std::errc::no_such_file_or_directory, std::errc::permission_denied,
                             std::errc::io_error}) {
        SCOPED_TRACE(std::make_error_code(error).message());
        EXPECT_CALL(*file_reader_, read(std::filesystem::path("/proc/stat")))
            .WillOnce(
                testing::Return(FileReadResult(std::unexpected(std::make_error_code(error)))));

        const auto result = reader.read();

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), CpuReadError::FileReadFailed);
    }
}

TEST_F(ProcCpuReaderImplTest, MapsInvalidFirstLineToInvalidDataWithoutSearchingLaterLines) {
    const ProcCpuReaderImpl reader(file_reader_);
    for (const std::string contents :
         {"", "\ncpu 1 2 3 4 5 6 7 8\n", "cpu0 1 2 3 4 5 6 7 8\ncpu 1 2 3 4 5 6 7 8\n", "cpu 1 2",
          "cpu invalid 2 3 4 5 6 7 8"}) {
        SCOPED_TRACE(contents);
        EXPECT_CALL(*file_reader_, read(std::filesystem::path("/proc/stat")))
            .WillOnce(testing::Return(FileReadResult(contents)));

        const auto result = reader.read();

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), CpuReadError::InvalidData);
    }
}

TEST_F(ProcCpuReaderImplTest, CollectsFreshCountersOnEachRead) {
    EXPECT_CALL(*file_reader_, read(std::filesystem::path("/proc/stat")))
        .WillOnce(testing::Return(FileReadResult("cpu 1 2 3 4 5 6 7 8")))
        .WillOnce(testing::Return(FileReadResult("cpu 11 22 33 44 55 66 77 88")));
    const ProcCpuReaderImpl reader(file_reader_);

    const auto first = reader.read();
    const auto second = reader.read();

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first->user, 1U);
    EXPECT_EQ(second->user, 11U);
}

TEST(ProcCpuReaderConstructorTest, RejectsNullFileReader) {
    EXPECT_THROW((void)ProcCpuReaderImpl(nullptr), std::invalid_argument);
}

}  // namespace
}  // namespace pulse
