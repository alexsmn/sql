#pragma once

#include "sql/types.h"

#include <string>

struct sqlite3_stmt;

namespace sql::sqlite3 {

class statement;

class field_view {
 public:
  field_type type() const;

  bool as_bool() const;
  int as_int() const;
  int64_t as_int64() const;
  double as_double() const;
  std::string_view as_string_view() const;
  std::string as_string() const;
  std::u16string as_string16() const;

 private:
  field_view(::sqlite3_stmt* stmt, int field_index);

  ::sqlite3_stmt* stmt_ = nullptr;
  int field_index_ = -1;

  friend class statement;
};

}  // namespace sql::sqlite3
