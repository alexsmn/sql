#include "sql/sqlite3/connection.h"

#include "sql/exception.h"
#include "sql/sqlite3/statement.h"

#include <cassert>
#include <format>
#include <sqlite3.h>

namespace sql::sqlite3 {

Connection::Connection() {}

Connection::~Connection() {
  Close();
}

void Connection::Open(const OpenParams& params) {
  assert(!db_);

  int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
  if (params.multithreaded)
    flags |= SQLITE_OPEN_NOMUTEX;

  int error = sqlite3_open_v2(
      reinterpret_cast<const char*>(params.path.u8string().c_str()), &db_,
      flags, nullptr);
  if (error != SQLITE_OK) {
    db_ = NULL;
    throw Exception{"Open error"};
  }

  if (params.exclusive_locking)
    Execute("PRAGMA locking_mode=EXCLUSIVE");

  if (params.journal_size_limit != -1) {
    Execute(
        std::format("PRAGMA journal_size_limit={}", params.journal_size_limit)
            .c_str());
  }
}

void Connection::Close() {
  begin_transaction_statement_.reset();
  commit_transaction_statement_.reset();
  rollback_transaction_statement_.reset();
  does_table_exist_statement_.reset();
  does_column_exist_statement_.reset();
  does_index_exist_statement_.reset();

  if (db_) {
    if (sqlite3_close(db_) != SQLITE_OK)
      throw Exception{sqlite3_errmsg(db_)};
    db_ = NULL;
  }
}

void Connection::Execute(const char* sql) {
  assert(db_);
  if (sqlite3_exec(db_, sql, NULL, NULL, NULL) != SQLITE_OK)
    throw Exception{sqlite3_errmsg(db_)};
}

bool Connection::DoesTableExist(const char* table_name) const {
  if (!does_table_exist_statement_.get()) {
    does_table_exist_statement_.reset(new Statement());
    does_table_exist_statement_->Init(*const_cast<Connection*>(this),
                                      "SELECT name FROM sqlite_master "
                                      "WHERE type='table' AND name=?");
  }

  does_table_exist_statement_->Bind(0, table_name);

  // Table exists if any row was returned.
  bool exists = does_table_exist_statement_->Step();

  does_table_exist_statement_->Reset();

  return exists;
}

bool Connection::DoesColumnExist(const char* table_name,
                                 const char* column_name) const {
  if (does_column_exist_table_name_ != table_name) {
    std::string sql = "PRAGMA TABLE_INFO(";
    sql += table_name;
    sql += ")";

    does_column_exist_table_name_ = table_name;
    does_column_exist_statement_.reset(new Statement());
    does_column_exist_statement_->Init(*const_cast<Connection*>(this),
                                       sql.c_str());
  }

  bool exists = false;
  while (does_table_exist_statement_->Step()) {
    if (does_column_exist_statement_->GetColumnString(1).compare(column_name) ==
        0) {
      exists = true;
      break;
    }
  }

  does_table_exist_statement_->Reset();

  return exists;
}

bool Connection::DoesIndexExist(const char* table_name,
                                const char* index_name) const {
  if (does_index_exist_table_name_ != table_name) {
    std::string sql = "PRAGMA INDEX_LIST(";
    sql += table_name;
    sql += ")";

    does_index_exist_table_name_ = table_name;
    does_index_exist_statement_.reset(new Statement());
    does_index_exist_statement_->Init(*const_cast<Connection*>(this),
                                      sql.c_str());
  }

  bool exists = false;
  while (does_index_exist_statement_->Step()) {
    if (does_index_exist_statement_->GetColumnString(1).compare(index_name) ==
        0) {
      exists = true;
      break;
    }
  }

  does_index_exist_statement_->Reset();

  return exists;
}

void Connection::BeginTransaction() {
  if (!begin_transaction_statement_.get()) {
    begin_transaction_statement_.reset(new Statement());
    begin_transaction_statement_->Init(*this, "BEGIN TRANSACTION");
  }

  begin_transaction_statement_->Run();
  begin_transaction_statement_->Reset();
}

void Connection::CommitTransaction() {
  if (!commit_transaction_statement_.get()) {
    commit_transaction_statement_.reset(new Statement());
    commit_transaction_statement_->Init(*this, "COMMIT");
  }

  commit_transaction_statement_->Run();
  commit_transaction_statement_->Reset();
}

void Connection::RollbackTransaction() {
  if (!rollback_transaction_statement_.get()) {
    rollback_transaction_statement_.reset(new Statement());
    rollback_transaction_statement_->Init(*this, "ROLLBACK");
  }

  rollback_transaction_statement_->Run();
  rollback_transaction_statement_->Reset();
}

int Connection::GetLastChangeCount() const {
  assert(db_);
  return sqlite3_changes(db_);
}

}  // namespace sql::sqlite3
