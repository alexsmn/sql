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
  if (!does_table_exist_statement_.get()) {
    does_table_exist_statement_.reset(new Statement());
    does_table_exist_statement_->Init(*const_cast<Connection*>(this),
                                      "SELECT FROM information_schema.tables "
                                      "WHERE table_schema='public' AND "
                                      "table_name=$1");
  }

  does_table_exist_statement_->Bind(0, table_name);

  // Table exists if any row was returned.
  bool exists = does_table_exist_statement_->Step();

  does_table_exist_statement_->Reset();

  return exists;
}

bool Connection::DoesColumnExist(std::string_view table_name,
                                 std::string_view column_name) const {
  if (!does_column_exist_statement_) {
    does_column_exist_statement_ = std::make_unique<Statement>();
    does_column_exist_statement_->Init(
        *const_cast<Connection*>(this),
        "SELECT FROM information_schema.columns WHERE table_schema='public' "
        "AND table_name=$1 AND column_name=$2");
  }

  does_column_exist_statement_->Bind(0, ToLowerCase(table_name));
  does_column_exist_statement_->Bind(1, ToLowerCase(column_name));

  bool exists = does_column_exist_statement_->Step();

  does_column_exist_statement_->Reset();

  return exists;
}

bool Connection::DoesIndexExist(std::string_view table_name,
                                std::string_view index_name) const {
  if (does_index_exist_table_name_ != table_name) {
    does_index_exist_table_name_ = table_name;
    does_index_exist_statement_.reset(new Statement());
    does_index_exist_statement_->Init(
        *const_cast<Connection*>(this),
        "SELECT FROM pg_indexes WHERE schemaname='public' "
        "AND tablename=$1 AND indexname=$2");
  }

  does_index_exist_statement_->Bind(0, ToLowerCase(table_name));
  does_index_exist_statement_->Bind(1, ToLowerCase(index_name));

  bool exists = does_index_exist_statement_->Step();

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
  assert(false);
  return 0;
}

std::string Connection::GenerateStatementName() {
  auto statement_id = next_statement_id_++;
  return std::format("stmt_{}", statement_id);
}

std::vector<Column> Connection::GetTableColumns(
    std::string_view table_name) const {
  if (!table_columns_statement_) {
    table_columns_statement_ = std::make_unique<Statement>();
    table_columns_statement_->Init(
        *const_cast<Connection*>(this),
        "SELECT column_name, data_type FROM information_schema.columns "
        "WHERE table_schema='public' AND table_name=$1");
  }

  table_columns_statement_->Bind(0, table_name);

  std::vector<Column> columns;

  while (table_columns_statement_->Step()) {
    auto column_name = table_columns_statement_->GetColumnString(0);
    auto column_data_type =
        ParsePostgresColumnType(table_columns_statement_->GetColumnString(1));
    // TODO: Handle properly.
    assert(column_data_type != COLUMN_TYPE_NULL);
    columns.emplace_back(std::move(column_name), column_data_type);
  }

  return columns;
}

}  // namespace sql::postgresql
