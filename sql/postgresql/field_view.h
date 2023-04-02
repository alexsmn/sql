#pragma once

#include "sql/postgresql/result.h"
#include "sql/types.h"

#include <cstdint>
#include <string>

namespace sql::postgresql {

class field_view {
 public:
  field_view(const result& result, int field_index);

  field_type GetType() const;

  bool get_bool() const;
  int get_int() const;
  std::int64_t get_int64() const;
  double get_double() const;
  std::string get_string() const;
  std::u16string get_string16() const;

 private:
  const result& result_;
  int field_index_;
};

}  // namespace sql::postgresql