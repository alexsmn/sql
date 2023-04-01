#pragma once

#include <gmock/gmock.h>

class ScopedTempDir {
 public:
  ScopedTempDir() {
    temp_dir_ = std::filesystem::temp_directory_path() / "sql";
  }

  ~ScopedTempDir() { std::filesystem::remove_all(temp_dir_); }

  const std::filesystem::path& get() const { return temp_dir_; }

 private:
  std::filesystem::path temp_dir_;
};
