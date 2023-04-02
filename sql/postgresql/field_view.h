#pragma once

#include "sql/postgresql/result.h"
#include "sql/types.h"

#include <cstdint>
#include <string>

namespace sql::postgresql {

class field_view {
 public:
  field_view(const result& result, int field_index);

  field_type type() const;

  bool as_bool() const;
  int as_int() const;
  std::int64_t as_int64() const;
  double as_double() const;
  std::string_view as_string_view() const;
  std::string as_string() const;
  std::u16string as_string16() const;

 private:
  const result& result_;
  int field_index_;
};

}  // namespace sql::postgresql