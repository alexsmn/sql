#pragma once

#include "sql/connection.h"
#include "sql/field_view.h"

#include <memory>
#include <string>

namespace sql {

class connection;

class statement {
 public:
  statement() = default;
  statement(connection& connection, std::string_view sql);

  statement(const statement&) = delete;
  statement& operator=(const statement&) = delete;

  statement(statement&& source) noexcept : model_{std::move(source.model_)} {}
  statement& operator=(statement&& source) noexcept {
    model_ = std::move(source.model_);
    return *this;
  }

  bool is_prepared() const;

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
  std::unique_ptr<connection::statement_model> model_;
};

}  // namespace sql
