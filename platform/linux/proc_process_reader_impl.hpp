
#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <vector>

#include "core/io/interfaces/file_reader.hpp"
#include "core/process/interfaces/process_reader.hpp"
#include "core/process/process_snapshot.hpp"

namespace pulse {

class ProcProcessReader final : public IProcessReader {
   public:
    explicit ProcProcessReader(std::shared_ptr<IFileReader> file_reader,
                               std::filesystem::path proc_path = "/proc");

    [[nodiscard]]
    std::expected<std::vector<ProcessSnapshot>, ProcessReadError> read() const override;

   private:
    std::shared_ptr<IFileReader> file_reader_;
    std::filesystem::path proc_path_;
};

}  // namespace pulse
