#include "platform/linux/proc_process_reader_impl.hpp"

#include <array>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#include "core/process/interfaces/process_reader.hpp"
#include "core/process/process_info.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "tests/mocks/mock_file_reader.hpp"

namespace pulse {
namespace {

using FileReadResult = std::expected<std::string, std::error_code>;

constexpr std::string_view valid_status =
    "Name: my worker\nState: S (sleeping)\nPid: 123\nVmRSS: 4096 kB\n";
constexpr std::string_view status_without_memory = "Name: other\nState: R (running)\nPid: 456\n";

class ProcProcessReaderTest : public testing::Test {
   protected:
    void SetUp() override {
        std::string pattern =
            (std::filesystem::temp_directory_path() / "pulse-process-test-XXXXXX").string();
        const char* directory = ::mkdtemp(pattern.data());
        ASSERT_NE(directory, nullptr);
        directory_ = directory;
    }

    void TearDown() override {
        if (!directory_.empty()) {
            std::error_code error;
            std::filesystem::remove_all(directory_, error);
            EXPECT_FALSE(error) << error.message();
        }
    }

    std::filesystem::path directory_;
    std::shared_ptr<testing::StrictMock<MockFileReader>> file_reader_ =
        std::make_shared<testing::StrictMock<MockFileReader>>();
};

TEST(ProcProcessReaderConstructorTest, RejectsNullFileReader) {
    EXPECT_THROW((void)ProcProcessReader(nullptr), std::invalid_argument);
}

TEST_F(ProcProcessReaderTest, ReturnsErrorForMissingDirectory) {
    const ProcProcessReader reader(file_reader_, directory_ / "missing");

    const auto result = reader.read();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProcessReadError::DirectoryReadFailed);
}

TEST_F(ProcProcessReaderTest, ReturnsErrorWhenRootIsARegularFile) {
    const auto path = directory_ / "file";
    std::ofstream file{path};
    ASSERT_TRUE(file.is_open());
    file.close();
    const ProcProcessReader reader(file_reader_, path);

    const auto result = reader.read();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProcessReadError::DirectoryReadFailed);
}

TEST_F(ProcProcessReaderTest, ReturnsEmptyVectorForEmptyDirectory) {
    const ProcProcessReader reader(file_reader_, directory_);

    const auto result = reader.read();

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST_F(ProcProcessReaderTest, ReadsStatusFromCustomRootAndMapsAllFields) {
    ASSERT_TRUE(std::filesystem::create_directory(directory_ / "123"));
    EXPECT_CALL(*file_reader_, read(directory_ / "123" / "status"))
        .WillOnce(testing::Return(FileReadResult{std::string{valid_status}}));
    const ProcProcessReader reader(file_reader_, directory_);

    const auto result = reader.read();

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1U);
    EXPECT_EQ(result->front().pid, 123);
    EXPECT_EQ(result->front().name, "my worker");
    EXPECT_EQ(result->front().state, 'S');
    EXPECT_THAT(result->front().memory_kb, testing::Optional(4096U));
}

TEST_F(ProcProcessReaderTest, FiltersNonnumericEntriesAndCollectsMultipleProcessesInAnyOrder) {
    for (const auto* name : std::array{"123", "456", "cpuinfo", "self", "12abc", "abc12", "-7"}) {
        ASSERT_TRUE(std::filesystem::create_directory(directory_ / name));
    }
    EXPECT_CALL(*file_reader_, read(directory_ / "123" / "status"))
        .WillOnce(testing::Return(FileReadResult{std::string{valid_status}}));
    EXPECT_CALL(*file_reader_, read(directory_ / "456" / "status"))
        .WillOnce(testing::Return(FileReadResult{std::string{status_without_memory}}));
    const ProcProcessReader reader(file_reader_, directory_);

    const auto result = reader.read();

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(*result, testing::UnorderedElementsAre(testing::Field(&ProcessInfo::pid, 123),
                                                       testing::Field(&ProcessInfo::pid, 456)));
}

TEST_F(ProcProcessReaderTest, IncludesProcessWithoutVmRss) {
    ASSERT_TRUE(std::filesystem::create_directory(directory_ / "456"));
    EXPECT_CALL(*file_reader_, read(directory_ / "456" / "status"))
        .WillOnce(testing::Return(FileReadResult{std::string{status_without_memory}}));
    const ProcProcessReader reader(file_reader_, directory_);

    const auto result = reader.read();

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1U);
    EXPECT_EQ(result->front().pid, 456);
    EXPECT_FALSE(result->front().memory_kb.has_value());
}

TEST_F(ProcProcessReaderTest, SkipsUnreadableProcessesAndKeepsValidOnes) {
    ASSERT_TRUE(std::filesystem::create_directory(directory_ / "123"));
    ASSERT_TRUE(std::filesystem::create_directory(directory_ / "456"));
    const ProcProcessReader reader(file_reader_, directory_);

    for (const auto error : std::array{std::errc::no_such_file_or_directory,
                                       std::errc::permission_denied, std::errc::io_error}) {
        SCOPED_TRACE(std::make_error_code(error).message());
        EXPECT_CALL(*file_reader_, read(directory_ / "123" / "status"))
            .WillOnce(
                testing::Return(FileReadResult{std::unexpected(std::make_error_code(error))}));
        EXPECT_CALL(*file_reader_, read(directory_ / "456" / "status"))
            .WillOnce(testing::Return(FileReadResult{std::string{status_without_memory}}));

        const auto result = reader.read();

        ASSERT_TRUE(result.has_value());
        EXPECT_THAT(*result, testing::ElementsAre(testing::Field(&ProcessInfo::pid, 456)));
    }
}

TEST_F(ProcProcessReaderTest, SkipsMalformedProcessesAndKeepsValidOnes) {
    ASSERT_TRUE(std::filesystem::create_directory(directory_ / "123"));
    ASSERT_TRUE(std::filesystem::create_directory(directory_ / "456"));
    const ProcProcessReader reader(file_reader_, directory_);

    for (const std::string_view content :
         std::array{"", "Name: incomplete\n", "Name: bad\nState: R\nPid: abc\n",
                    "Name: bad\nState: R\nPid: 123\nVmRSS: 1 MB\n"}) {
        SCOPED_TRACE(content);
        EXPECT_CALL(*file_reader_, read(directory_ / "123" / "status"))
            .WillOnce(testing::Return(FileReadResult{std::string{content}}));
        EXPECT_CALL(*file_reader_, read(directory_ / "456" / "status"))
            .WillOnce(testing::Return(FileReadResult{std::string{status_without_memory}}));

        const auto result = reader.read();

        ASSERT_TRUE(result.has_value());
        EXPECT_THAT(*result, testing::ElementsAre(testing::Field(&ProcessInfo::pid, 456)));
    }
}

TEST_F(ProcProcessReaderTest, ReturnsEmptyVectorWhenAllProcessesAreSkipped) {
    ASSERT_TRUE(std::filesystem::create_directory(directory_ / "123"));
    EXPECT_CALL(*file_reader_, read(directory_ / "123" / "status"))
        .WillOnce(testing::Return(FileReadResult{std::string{}}));
    const ProcProcessReader reader(file_reader_, directory_);

    const auto result = reader.read();

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST_F(ProcProcessReaderTest, ReadsFreshContentsOnEachCall) {
    ASSERT_TRUE(std::filesystem::create_directory(directory_ / "123"));
    EXPECT_CALL(*file_reader_, read(directory_ / "123" / "status"))
        .WillOnce(testing::Return(FileReadResult{std::string{valid_status}}))
        .WillOnce(
            testing::Return(FileReadResult{"Name: changed\nState: R\nPid: 123\nVmRSS: 0 kB\n"}));
    const ProcProcessReader reader(file_reader_, directory_);

    const auto first = reader.read();
    const auto second = reader.read();

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    ASSERT_EQ(first->size(), 1U);
    ASSERT_EQ(second->size(), 1U);
    EXPECT_EQ(first->front().name, "my worker");
    EXPECT_THAT(first->front().memory_kb, testing::Optional(4096U));
    EXPECT_EQ(second->front().name, "changed");
    EXPECT_EQ(second->front().state, 'R');
    EXPECT_THAT(second->front().memory_kb, testing::Optional(0U));
}

}  // namespace
}  // namespace pulse
