#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include "gtest/gtest.h"
#include "platform/linux/file_reader_impl.hpp"

namespace pulse {
namespace {

class FileReaderTest : public testing::Test {
   protected:
    void SetUp() override {
        std::string pattern =
            (std::filesystem::temp_directory_path() / "pulse-test-XXXXXX").string();
        const char* directory = ::mkdtemp(pattern.data());
        ASSERT_NE(directory, nullptr);
        directory_ = directory;
        path_ = directory_ / "input";
    }

    void TearDown() override {
        if (!directory_.empty()) {
            std::error_code error;
            std::filesystem::remove_all(directory_, error);
            EXPECT_FALSE(error) << error.message();
        }
    }

    void writeFile(const std::string& contents) const {
        std::ofstream file(path_, std::ios::binary);
        ASSERT_TRUE(file.is_open());
        file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        file.close();
        ASSERT_TRUE(file.good());
    }

    std::filesystem::path directory_;
    std::filesystem::path path_;
    FileReaderImpl reader_;
};

TEST_F(FileReaderTest, PreservesWhitespaceAndNewlines) {
    const std::string contents = "  first\tline\nsecond line\r\n\n";
    ASSERT_NO_FATAL_FAILURE(writeFile(contents));

    const auto result = reader_.read(path_);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, contents);
}

TEST_F(FileReaderTest, ReadsEmptyFileSuccessfully) {
    ASSERT_NO_FATAL_FAILURE(writeFile(""));

    const auto result = reader_.read(path_);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST_F(FileReaderTest, PreservesEmbeddedNullBytes) {
    const std::string contents("a\0b\0c", 5);
    ASSERT_NO_FATAL_FAILURE(writeFile(contents));

    const auto result = reader_.read(path_);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, contents);
}

TEST_F(FileReaderTest, ReadsEntireLargeFileWithoutTrailingNewline) {
    constexpr std::size_t content_size = 100000;
    const std::string contents(content_size, 'x');
    ASSERT_NO_FATAL_FAILURE(writeFile(contents));

    const auto result = reader_.read(path_);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, contents);
}

TEST_F(FileReaderTest, ReturnsErrorForMissingFile) {
    const auto result = reader_.read(path_);

    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(result.error());
}

TEST_F(FileReaderTest, ReturnsErrorWhenReadingDirectory) {
    const auto result = reader_.read(directory_);

    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(result.error());
}

}  // namespace
}  // namespace pulse
