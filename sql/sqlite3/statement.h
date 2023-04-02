#pragma once

#include "sql/types.h"

#include <string>

struct sqlite3_stmt;

namespace sql::sqlite3 {

class connection;

class statement {
 public:
  statement() = default;
  statement(connection& connection, std::string_view sql);
  ~statement();

  statement(const statement&) = delete;
  statement& operator=(const statement&) = delete;

  bool is_prepared() const { return !!stmt_; };

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

  bool get_bool(unsigned column) const;
  int get_int(unsigned column) const;
  int64_t get_int64(unsigned column) const;
  double get_double(unsigned column) const;
  std::string get_string(unsigned column) const;
  std::u16string get_string16(unsigned column) const;

  void query();
  bool next();
  void reset();

  void close();

 private:
  connection* connection_;
  ::sqlite3_stmt* stmt_;
};

}  // namespace sql::sqlite3
