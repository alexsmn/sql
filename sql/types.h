#pragma once

#include <filesystem>
#include <string>

namespace sql {

enum class field_type { INTEGER = 1, FLOAT = 2, TEXT = 3, BLOB = 4, EMPTY = 5 };

struct open_params {
  std::string driver;
  std::filesystem::path path;
  std::string connection_string;
  bool exclusive_locking = false;
  bool multithreaded = false;
  int journal_size_limit = -1;
};

struct field_info {
  std::string name;
  field_type type;
};

}  // namespace sql
