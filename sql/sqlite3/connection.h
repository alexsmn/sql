#pragma once

#include <memory>
#include <string>

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

  void Execute(const char* sql);

  void BeginTransaction();
  void CommitTransaction();
  void RollbackTransaction();

  int GetLastChangeCount() const;

  bool DoesTableExist(const char* table_name) const;
  bool DoesColumnExist(const char* table_name, const char* column_name) const;
  bool DoesIndexExist(const char* table_name, const char* index_name) const;

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
