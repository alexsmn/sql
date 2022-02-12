#pragma once

#include "sql/connection.h"
#include "sql/sqlite3/statement.h"

#include <string>

namespace sql {

class Connection;

class Statement {
 public:
  Statement() = default;

  Statement(const Statement&) = delete;
  Statement& operator=(const Statement&) = delete;

  Statement(Statement&& source) noexcept
      : sqlite3_statement_{std::move(source.sqlite3_statement_)} {}
  Statement& operator=(Statement&& source) noexcept {
    sqlite3_statement_ = std::move(source.sqlite3_statement_);
    return *this;
  }

  bool IsInitialized() const { return sqlite3_statement_.IsInitialized(); };

  void Init(Connection& connection, const char* sql) {
    sqlite3_statement_.Init(connection.sqlite3_connection_, sql);
  }

  void BindNull(unsigned column) { sqlite3_statement_.BindNull(column); }
  void Bind(unsigned column, bool value) {
    sqlite3_statement_.Bind(column, value);
  }
  void Bind(unsigned column, int value) {
    sqlite3_statement_.Bind(column, value);
  }
  void Bind(unsigned column, int64_t value) {
    sqlite3_statement_.Bind(column, value);
  }
  void Bind(unsigned column, double value) {
    sqlite3_statement_.Bind(column, value);
  }
  void Bind(unsigned column, const char* value) {
    sqlite3_statement_.Bind(column, value);
  }
  void Bind(unsigned column, const std::string& value) {
    sqlite3_statement_.Bind(column, value);
  }
  void Bind(unsigned column, const std::wstring& value) {
    sqlite3_statement_.Bind(column, value);
  }

  size_t GetColumnCount() const { return sqlite3_statement_.GetColumnCount(); }
  ColumnType GetColumnType(unsigned column) const {
    return sqlite3_statement_.GetColumnType(column);
  }

  bool GetColumnBool(unsigned column) const {
    return sqlite3_statement_.GetColumnBool(column);
  }
  int GetColumnInt(unsigned column) const {
    return sqlite3_statement_.GetColumnInt(column);
  }
  int64_t GetColumnInt64(unsigned column) const {
    return sqlite3_statement_.GetColumnInt64(column);
  }
  double GetColumnDouble(unsigned column) const {
    return sqlite3_statement_.GetColumnDouble(column);
  }
  std::string GetColumnString(unsigned column) const {
    return sqlite3_statement_.GetColumnString(column);
  }
  std::wstring GetColumnString16(unsigned column) const {
    return sqlite3_statement_.GetColumnString16(column);
  }

  void Run() { sqlite3_statement_.Run(); }
  bool Step() { return sqlite3_statement_.Step(); }
  void Reset() { sqlite3_statement_.Reset(); }

  void Close() { sqlite3_statement_.Close(); }

 private:
  sqlite3::Statement sqlite3_statement_;
};

}  // namespace sql