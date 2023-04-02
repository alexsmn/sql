#pragma once

#include <format>
#include <gmock/gmock.h>
#include <random>

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
    std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> distrib(0, std::numeric_limits<int>::max());

    for (int count = 0; count < 15; ++count) {
      auto new_dir = base_dir / std::format("scoped_temp_dir_{}", distrib(gen));

      std::error_code ec;
      if (std::filesystem::create_directory(new_dir, ec)) {
        return new_dir;
      }
    }

    throw std::runtime_error("Cannot create a temp directory");
  }

  std::filesystem::path temp_dir_;
};
