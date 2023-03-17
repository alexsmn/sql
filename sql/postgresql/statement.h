#pragma once

#include "sql/types.h"

#include <libpq-fe.h>
#include <libpq/libpq-fs.h>
#include <string>
#include <vector>

namespace sql::postgresql {

class Connection;

class Statement {
 public:
  Statement() = default;
  ~Statement();

  Statement(const Statement&) = delete;
  Statement& operator=(const Statement&) = delete;

  bool IsInitialized() const { return !name_.empty(); };

  void Init(Connection& connection, std::string_view sql);

  void BindNull(unsigned column);
  void Bind(unsigned column, bool value);
  void Bind(unsigned column, int value);
  void Bind(unsigned column, int64_t value);
  void Bind(unsigned column, double value);
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
  struct Param {
    Oid type;
    std::vector<char> value;
  };

  Param& GetParam(unsigned column);

  void Prepare();
  void Execute(bool single_row);

  Connection* connection_ = nullptr;
  ::PGconn* conn_ = nullptr;
  ::PGresult* result_ = nullptr;

  std::string name_;
  std::string sql_;

  std::vector<Param> params_;

  bool prepared_ = false;
  bool executed_ = false;
};

}  // namespace sql::postgresql
