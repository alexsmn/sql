#pragma once

#include "sql/types.h"

#include <memory>
#include <string>
#include <vector>

struct sqlite3;

namespace sql {
struct OpenParams;
}

namespace sql::sqlite3 {

class Statement;

class Connection {
 public:
  Connection();
  ~Connection();

  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  void Open(const OpenParams& params);
  void Close();

  void Execute(std::string_view sql);

  void BeginTransaction();
  void CommitTransaction();
  void RollbackTransaction();

  int GetLastChangeCount() const;

  bool DoesTableExist(std::string_view table_name) const;
  bool DoesColumnExist(std::string_view table_name,
                       std::string_view column_name) const;
  bool DoesIndexExist(std::string_view table_name,
                      std::string_view index_name) const;

  std::vector<Column> GetTableColumns(std::string_view table_name) const;

 private:
  ::sqlite3* db_ = nullptr;

  mutable std::unique_ptr<Statement> begin_transaction_statement_;
  mutable std::unique_ptr<Statement> commit_transaction_statement_;
  mutable std::unique_ptr<Statement> rollback_transaction_statement_;

  mutable std::unique_ptr<Statement> does_table_exist_statement_;
  mutable std::unique_ptr<Statement> does_column_exist_statement_;
  mutable std::unique_ptr<Statement> does_index_exist_statement_;
  mutable std::string does_column_exist_table_name_;
  mutable std::string does_index_exist_table_name_;

  friend class Statement;
};

}  // namespace sql::sqlite3
