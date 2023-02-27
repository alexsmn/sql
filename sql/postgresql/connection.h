#pragma once

#include <filesystem>
#include <memory>

struct sqlite3;

namespace sql::postgresql {

class Statement;

class Connection {
 public:
  Connection();
  ~Connection();

  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  void set_exclusive_locking() { exclusive_locking_ = true; }
  void set_journal_size_limit(int limit) { journal_size_limit_ = limit; }

  void Open(const std::filesystem::path& path);
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
  ::sqlite3* db_;

  bool exclusive_locking_;
  int journal_size_limit_;

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

}  // namespace sql::postgresql
