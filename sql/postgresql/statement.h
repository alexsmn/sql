#pragma once

#include "sql/types.h"

#include <string>

struct sqlite3_stmt;

namespace sql::postgresql {

class Connection;

class Statement {
 public:
  Statement() = default;
  ~Statement();

  Statement(const Statement&) = delete;
  Statement& operator=(const Statement&) = delete;

  Statement(Statement&& source) noexcept;
  Statement& operator=(Statement&& source) noexcept;

  bool IsInitialized() const { return !!stmt_; };

  void Init(Connection& connection, const char* sql);

  void BindNull(unsigned column);
  void Bind(unsigned column, bool value);
  void Bind(unsigned column, int value);
  void Bind(unsigned column, int64_t value);
  void Bind(unsigned column, double value);
  void Bind(unsigned column, const char* value);
  void Bind(unsigned column, const std::string& value);
  void Bind(unsigned column, const std::u16string& value);

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
  Connection* connection_ = nullptr;
  ::sqlite3_stmt* stmt_ = nullptr;
};

}  // namespace sql::postgresql
