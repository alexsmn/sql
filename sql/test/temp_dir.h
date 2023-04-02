#pragma once

#include <format>
#include <gmock/gmock.h>

class ScopedTempDir {
 public:
  ScopedTempDir() {
    temp_dir_ = CreateTemporaryDirInDir(std::filesystem::temp_directory_path());
  }

  ~ScopedTempDir() { std::filesystem::remove_all(temp_dir_); }

  const std::filesystem::path& get() const { return temp_dir_; }

 private:
  static std::filesystem::path CreateTemporaryDirInDir(
      const std::filesystem::path& base_dir) {
    for (int count = 0; count < 15; ++count) {
      auto new_dir = base_dir / std::format("scoped_temp_dir_{}", std::rand());

      std::error_code ec;
      if (std::filesystem::create_directory(new_dir, ec)) {
        return new_dir;
      }
    }

    throw std::runtime_error("Cannot create a temp directory");
  }

  std::filesystem::path temp_dir_;
};
