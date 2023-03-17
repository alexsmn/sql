#pragma once

#include "sql/types.h"

#include <memory>
#include <string>
#include <vector>

typedef struct pg_conn PGconn;

namespace sql {
struct OpenParams;
}

namespace sql::postgresql {

// TODO: std code standard.

class Statement;

class Connection {
 public:
  Connection() = default;
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

  // TODO: string_view

  bool DoesTableExist(std::string_view table_name) const;
  bool DoesColumnExist(std::string_view table_name,
                       std::string_view column_name) const;
  bool DoesIndexExist(std::string_view table_name,
                      std::string_view index_name) const;

  std::vector<Column> GetTableColumns(std::string_view table_name) const;

 private:
  std::string GenerateStatementName();

  ::PGconn* conn_ = nullptr;

  mutable std::unique_ptr<Statement> begin_transaction_statement_;
  mutable std::unique_ptr<Statement> commit_transaction_statement_;
  mutable std::unique_ptr<Statement> rollback_transaction_statement_;

  mutable std::unique_ptr<Statement> table_columns_statement_;
  mutable std::unique_ptr<Statement> does_table_exist_statement_;
  mutable std::unique_ptr<Statement> does_column_exist_statement_;
  mutable std::unique_ptr<Statement> does_index_exist_statement_;
  mutable std::string does_index_exist_table_name_;

  std::atomic<int> next_statement_id_ = 0;

  friend class Statement;
};

}  // namespace sql::postgresql
