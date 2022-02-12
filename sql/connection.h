#pragma once

#include <filesystem>

#include "sql/sqlite3/connection.h"

namespace sql {

class Statement;

class Connection {
 public:
  Connection() = default;

  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  void set_exclusive_locking() { sqlite3_connection_.set_exclusive_locking(); }
  void set_journal_size_limit(int limit) {
    sqlite3_connection_.set_journal_size_limit(limit);
  }

  void Open(const std::filesystem::path& path) {
    sqlite3_connection_.Open(path);
  }
  void Close() { sqlite3_connection_.Close(); }

  void Execute(const char* sql) { sqlite3_connection_.Execute(sql); }

  void BeginTransaction() { sqlite3_connection_.BeginTransaction(); }
  void CommitTransaction() { sqlite3_connection_.CommitTransaction(); }
  void RollbackTransaction() { sqlite3_connection_.RollbackTransaction(); }

  int GetLastChangeCount() const {
    return sqlite3_connection_.GetLastChangeCount();
  }

  bool DoesTableExist(const char* table_name) const {
    return sqlite3_connection_.DoesTableExist(table_name);
  }
  bool DoesColumnExist(const char* table_name, const char* column_name) const {
    return sqlite3_connection_.DoesColumnExist(table_name, column_name);
  }
  bool DoesIndexExist(const char* table_name, const char* index_name) const {
    return sqlite3_connection_.DoesIndexExist(table_name, index_name);
  }

 private:
  friend class Statement;

  sqlite3::Connection sqlite3_connection_;
};

}  // namespace sql
