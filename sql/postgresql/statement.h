#pragma once

#include "sql/postgresql/field_view.h"
#include "sql/postgresql/result.h"
#include "sql/types.h"

#include <boost/container/small_vector.hpp>
#include <string>
#include <vector>

namespace sql::postgresql {

class connection;

class statement {
 public:
  statement() = default;
  statement(connection& connection, std::string_view sql);
  ~statement();

  statement(const statement&) = delete;
  statement& operator=(const statement&) = delete;

  bool is_prepared() const { return !name_.empty(); };

  void prepare(connection& connection, std::string_view sql);

  void bind_null(unsigned column);
  void bind(unsigned column, bool value);
  void bind(unsigned column, int value);
  void bind(unsigned column, int64_t value);
  void bind(unsigned column, double value);
  // Add explicit c-string parameters to avoid implicit cast to `bool`.
  void bind(unsigned column, const char* value);
  void bind(unsigned column, const char16_t* value);
  void bind(unsigned column, std::string_view value);
  void bind(unsigned column, std::u16string_view value);

  size_t field_count() const;
  field_type field_type(unsigned column) const;
  field_view at(unsigned column) const;

  void query();
  bool next();
  void reset();

  void close();

 private:
  using ParamBuffer = boost::container::small_vector<char, 8>;

  struct Param {
    // The parameter type in assigned on `create`.
    Oid type = InvalidOid;
    // Empty buffer is used for null values.
    ParamBuffer buffer;
  };

  void query(bool single_row);

  connection* connection_ = nullptr;
  ::PGconn* conn_ = nullptr;
  result result_;

  std::string name_;

  // For debugging purposes only.
#ifndef NDEBUG
  std::string sql_;
#endif

  std::vector<Param> params_;

  bool executed_ = false;
};

}  // namespace sql::postgresql
