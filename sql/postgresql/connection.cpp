#include "sql/postgresql/connection.h"

#include "sql/exception.h"
#include "sql/postgresql/postgres_util.h"
#include "sql/postgresql/statement.h"

#include <boost/algorithm/string.hpp>
#include <cassert>
#include <format>
#include <libpq-fe.h>
#include <libpq/libpq-fs.h>

namespace sql::postgresql {

namespace {

// A convenice function since |boost::algorithm::to_lower_copy| doesn't work
// with |std::string_view|.
std::string ToLowerCase(std::string_view str) {
  std::string result{str};
  boost::algorithm::to_lower(result);
  return result;
}

}  // namespace

Connection::Connection(const OpenParams& params) {
  Open(params);
}

Connection::~Connection() {
  Close();
}

void Connection::Open(const OpenParams& params) {
  assert(!conn_);

  PGconn* conn = PQconnectdb(params.connection_string.c_str());
  if (conn == nullptr || PQstatus(conn) != CONNECTION_OK) {
    std::string message = PQerrorMessage(conn);
    PQfinish(conn);
    throw Exception{message};
  }

  conn_ = conn;
}

void Connection::Close() {
  begin_transaction_statement_.reset();
  commit_transaction_statement_.reset();
  rollback_transaction_statement_.reset();

  table_columns_statement_.reset();
  does_table_exist_statement_.reset();
  does_column_exist_statement_.reset();
  does_index_exist_statement_.reset();

  if (conn_) {
    PQfinish(conn_);
    conn_ = nullptr;
  }
}

void Connection::Execute(std::string_view sql) {
  CheckPostgresResult(PQexec(conn_, std::string{sql}.c_str()));
}

bool Connection::DoesTableExist(std::string_view table_name) const {
  if (!does_table_exist_statement_) {
    does_table_exist_statement_ =
        std::make_unique<Statement>(*const_cast<Connection*>(this),
                                    "SELECT FROM information_schema.tables "
                                    "WHERE table_schema='public' AND "
                                    "table_name=?");
  }

  does_table_exist_statement_->Bind(0, ToLowerCase(table_name));

  // Table exists if any row was returned.
  bool exists = does_table_exist_statement_->Step();

  does_table_exist_statement_->Reset();

  return exists;
}

bool Connection::DoesColumnExist(std::string_view table_name,
                                 std::string_view column_name) const {
  if (!does_column_exist_statement_) {
    does_column_exist_statement_ = std::make_unique<Statement>(
        *const_cast<Connection*>(this),
        "SELECT FROM information_schema.columns WHERE table_schema='public' "
        "AND table_name=? AND column_name=?");
  }

  does_column_exist_statement_->Bind(0, ToLowerCase(table_name));
  does_column_exist_statement_->Bind(1, ToLowerCase(column_name));

  bool exists = does_column_exist_statement_->Step();

  does_column_exist_statement_->Reset();

  return exists;
}

bool Connection::DoesIndexExist(std::string_view table_name,
                                std::string_view index_name) const {
  if (!does_index_exist_statement_) {
    does_index_exist_statement_ = std::make_unique<Statement>(
        *const_cast<Connection*>(this),
        "SELECT FROM pg_indexes WHERE schemaname='public' "
        "AND tablename=? AND indexname=?");
  }

  does_index_exist_statement_->Bind(0, ToLowerCase(table_name));
  does_index_exist_statement_->Bind(1, ToLowerCase(index_name));

  bool exists = does_index_exist_statement_->Step();

  does_index_exist_statement_->Reset();

  return exists;
}

void Connection::BeginTransaction() {
  if (!begin_transaction_statement_) {
    begin_transaction_statement_ =
        std::make_unique<Statement>(*this, "BEGIN TRANSACTION");
  }

  begin_transaction_statement_->Run();
  begin_transaction_statement_->Reset();
}

void Connection::CommitTransaction() {
  if (!commit_transaction_statement_) {
    commit_transaction_statement_ =
        std::make_unique<Statement>(*this, "COMMIT");
  }

  commit_transaction_statement_->Run();
  commit_transaction_statement_->Reset();
}

void Connection::RollbackTransaction() {
  if (!rollback_transaction_statement_) {
    rollback_transaction_statement_ =
        std::make_unique<Statement>(*this, "ROLLBACK");
  }

  rollback_transaction_statement_->Run();
  rollback_transaction_statement_->Reset();
}

int Connection::GetLastChangeCount() const {
  return last_change_count_;
}

std::string Connection::GenerateStatementName() {
  auto statement_id = next_statement_id_++;
  return std::format("stmt_{}", statement_id);
}

std::vector<Column> Connection::GetTableColumns(
    std::string_view table_name) const {
  if (!table_columns_statement_) {
    table_columns_statement_ = std::make_unique<Statement>(
        *const_cast<Connection*>(this),
        "SELECT column_name, data_type FROM information_schema.columns "
        "WHERE table_schema='public' AND table_name=?");
  }

  table_columns_statement_->Bind(0, ToLowerCase(table_name));

  std::vector<Column> columns;

  while (table_columns_statement_->Step()) {
    auto column_name = table_columns_statement_->GetColumnString(0);
    auto column_type = table_columns_statement_->GetColumnString(1);
    auto parsed_column_type = ParsePostgresColumnType(column_type);
    // TODO: Handle properly.
    assert(parsed_column_type != COLUMN_TYPE_NULL);
    columns.emplace_back(std::move(column_name), parsed_column_type);
  }

  table_columns_statement_->Reset();

  return columns;
}

}  // namespace sql::postgresql
