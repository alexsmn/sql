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

connection::connection(const open_params& params) {
  open(params);
}

connection::~connection() {
  close();
}

void connection::open(const open_params& params) {
  assert(!conn_);

  PGconn* conn = PQconnectdb(params.connection_string.c_str());
  if (conn == nullptr || PQstatus(conn) != CONNECTION_OK) {
    std::string message = PQerrorMessage(conn);
    PQfinish(conn);
    throw Exception{message};
  }

  conn_ = conn;
}

void connection::close() {
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

void connection::query(std::string_view sql) {
  CheckPostgresResult(PQexec(conn_, std::string{sql}.c_str()));
}

bool connection::table_exists(std::string_view table_name) const {
  if (!does_table_exist_statement_) {
    does_table_exist_statement_ =
        std::make_unique<statement>(*const_cast<connection*>(this),
                                    "SELECT FROM information_schema.tables "
                                    "WHERE table_schema='public' AND "
                                    "table_name=?");
  }

  does_table_exist_statement_->bind(0, ToLowerCase(table_name));

  // Table exists if any row was returned.
  bool exists = does_table_exist_statement_->next();

  does_table_exist_statement_->reset();

  return exists;
}

bool connection::field_exists(std::string_view table_name,
                              std::string_view column_name) const {
  if (!does_column_exist_statement_) {
    does_column_exist_statement_ = std::make_unique<statement>(
        *const_cast<connection*>(this),
        "SELECT FROM information_schema.columns WHERE table_schema='public' "
        "AND table_name=? AND column_name=?");
  }

  does_column_exist_statement_->bind(0, ToLowerCase(table_name));
  does_column_exist_statement_->bind(1, ToLowerCase(column_name));

  bool exists = does_column_exist_statement_->next();

  does_column_exist_statement_->reset();

  return exists;
}

bool connection::index_exists(std::string_view table_name,
                              std::string_view index_name) const {
  if (!does_index_exist_statement_) {
    does_index_exist_statement_ = std::make_unique<statement>(
        *const_cast<connection*>(this),
        "SELECT FROM pg_indexes WHERE schemaname='public' "
        "AND tablename=? AND indexname=?");
  }

  does_index_exist_statement_->bind(0, ToLowerCase(table_name));
  does_index_exist_statement_->bind(1, ToLowerCase(index_name));

  bool exists = does_index_exist_statement_->next();

  does_index_exist_statement_->reset();

  return exists;
}

void connection::start() {
  if (!begin_transaction_statement_) {
    begin_transaction_statement_ =
        std::make_unique<statement>(*this, "BEGIN TRANSACTION");
  }

  begin_transaction_statement_->query();
  begin_transaction_statement_->reset();
}

void connection::commit() {
  if (!commit_transaction_statement_) {
    commit_transaction_statement_ =
        std::make_unique<statement>(*this, "COMMIT");
  }

  commit_transaction_statement_->query();
  commit_transaction_statement_->reset();
}

void connection::rollback() {
  if (!rollback_transaction_statement_) {
    rollback_transaction_statement_ =
        std::make_unique<statement>(*this, "ROLLBACK");
  }

  rollback_transaction_statement_->query();
  rollback_transaction_statement_->reset();
}

int connection::last_change_count() const {
  return last_change_count_;
}

std::string connection::GenerateStatementName() {
  auto statement_id = next_statement_id_++;
  return std::format("stmt_{}", statement_id);
}

std::vector<field_info> connection::table_fields(
    std::string_view table_name) const {
  if (!table_columns_statement_) {
    table_columns_statement_ = std::make_unique<statement>(
        *const_cast<connection*>(this),
        "SELECT column_name, data_type FROM information_schema.columns "
        "WHERE table_schema='public' AND table_name=?");
  }

  table_columns_statement_->bind(0, ToLowerCase(table_name));

  std::vector<field_info> columns;

  while (table_columns_statement_->next()) {
    auto column_name = table_columns_statement_->get_string(0);
    auto column_type = table_columns_statement_->get_string(1);
    auto parsed_column_type = ParsePostgresColumnType(column_type);
    assert(parsed_column_type != field_type::EMPTY);
    columns.emplace_back(std::move(column_name), parsed_column_type);
  }

  table_columns_statement_->reset();

  return columns;
}

}  // namespace sql::postgresql
