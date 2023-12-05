#pragma once

#include "sql/connection.h"
#include "sql/types.h"

namespace sql {

class statement;

class field_view {
 public:
  bool is_null() const { return type() == field_type::EMPTY; }
  field_type type() const { return statement_.type(field_index_); }

  bool as_bool() const { return statement_.as_bool(field_index_); }
  int as_int() const { return statement_.as_int(field_index_); }
  int64_t as_int64() const { return statement_.as_int64(field_index_); }
  double as_double() const { return statement_.as_double(field_index_); }
  std::string_view as_string_view() const {
    return statement_.as_string_view(field_index_);
  }
  std::string as_string() const { return statement_.as_string(field_index_); }
  std::u16string as_string16() const {
    return statement_.as_string16(field_index_);
  }

 private:
  field_view(connection::statement_model& statement, int field_index)
      : statement_{statement}, field_index_{field_index} {}

  connection::statement_model& statement_;
  int field_index_;

  friend class statement;
};

}  // namespace sql
