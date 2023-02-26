#pragma once

#include <filesystem>

namespace sql {

enum ColumnType {
  COLUMN_TYPE_INTEGER = 1,
  COLUMN_TYPE_FLOAT = 2,
  COLUMN_TYPE_TEXT = 3,
  COLUMN_TYPE_BLOB = 4,
  COLUMN_TYPE_NULL = 5
};

struct OpenParams {
  std::filesystem::path path;
  bool exclusive_locking = false;
  bool multithreaded = false;
  int journal_size_limit = -1;
};

}  // namespace sql
