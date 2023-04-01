#pragma once

#include "sql/connection.h"

#include <memory>
#include <string>

namespace sql {

class Connection;

class Statement {
 public:
  Statement() = default;
  Statement(Connection& connection, std::string_view sql);

  Statement(const Statement&) = delete;
  Statement& operator=(const Statement&) = delete;

  Statement(Statement&& source) noexcept : model_{std::move(source.model_)} {}
  Statement& operator=(Statement&& source) noexcept {
    model_ = std::move(source.model_);
    return *this;
  }

  bool IsInitialized() const;

  void Init(Connection& connection, std::string_view sql);

  void BindNull(unsigned column);
  void Bind(unsigned column, bool value);
  void Bind(unsigned column, int value);
  void Bind(unsigned column, int64_t value);
  void Bind(unsigned column, double value);
  // Add explicit c-string parameters to avoid implicit cast to `bool`.
  void Bind(unsigned column, const char* value);
  void Bind(unsigned column, const char16_t* value);
  void Bind(unsigned column, std::string_view value);
  void Bind(unsigned column, std::u16string_view value);

  size_t GetColumnCount() const;
  ColumnType GetColumnType(unsigned column) const;

  bool GetColumnBool(unsigned column) const;
  int GetColumnInt(unsigned column) const;
  int64_t GetColumnInt64(unsigned column) const;
  double GetColumnDouble(unsigned column) const;
  std::string GetColumnString(unsigned column) const;
  std::u16string GetColumnString16(unsigned column) const;

  void Run();
  bool Step();
  void Reset();

  void Close();

 private:
  std::unique_ptr<Connection::StatementModel> model_;
};

}  // namespace sql
