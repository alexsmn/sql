#pragma once

#include "sql/types.h"

#include <string>

struct sqlite3_stmt;

namespace sql::sqlite3 {

class Connection;

class Statement {
 public:
  Statement();
  ~Statement();

  Statement(const Statement&) = delete;
  Statement& operator=(const Statement&) = delete;

  Statement(Statement&& source);
  Statement& operator=(Statement&& source);

  bool IsInitialized() const { return !!stmt_; };

  void Init(Connection& connection, const char* sql);

  void BindNull(unsigned column);
  void Bind(unsigned column, bool value);
  void Bind(unsigned column, int value);
  void Bind(unsigned column, int64_t value);
  void Bind(unsigned column, double value);
  void Bind(unsigned column, const char* value);
  void Bind(unsigned column, const std::string& value);
  void Bind(unsigned column, const std::wstring& value);

  size_t GetColumnCount() const;
  ColumnType GetColumnType(unsigned column) const;

  bool GetColumnBool(unsigned column) const;
  int GetColumnInt(unsigned column) const;
  int64_t GetColumnInt64(unsigned column) const;
  double GetColumnDouble(unsigned column) const;
  std::string GetColumnString(unsigned column) const;
  std::wstring GetColumnString16(unsigned column) const;

  void Run();
  bool Step();
  void Reset();

  void Close();

 private:
  Connection* connection_;
  ::sqlite3_stmt* stmt_;
};

}  // namespace sql::sqlite3