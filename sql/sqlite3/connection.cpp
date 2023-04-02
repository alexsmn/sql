#include "sql/sqlite3/connection.h"

#include "sql/exception.h"
#include "sql/sqlite3/sqlite_util.h"
#include "sql/sqlite3/statement.h"

#include <cassert>
#include <format>
#include <sqlite3.h>

namespace sql::sqlite3 {

connection::connection(const open_params& params) {
  open(params);
}

connection::~connection() {
  close();
}

void connection::open(const open_params& params) {
  assert(!db_);

  int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
  if (params.multithreaded)
    flags |= SQLITE_OPEN_NOMUTEX;

  int error = sqlite3_open_v2(
      reinterpret_cast<const char*>(params.path.u8string().c_str()), &db_,
      flags, nullptr);
  if (error != SQLITE_OK) {
    db_ = nullptr;
    throw Exception{"open error"};
  }

  if (params.exclusive_locking)
    query("PRAGMA locking_mode=EXCLUSIVE");

  if (params.journal_size_limit != -1) {
    query(
        std::format("PRAGMA journal_size_limit={}", params.journal_size_limit));
  }
}

void connection::close() {
  begin_transaction_statement_.reset();
  commit_transaction_statement_.reset();
  rollback_transaction_statement_.reset();
  does_table_exist_statement_.reset();
  does_column_exist_statement_.reset();
  does_index_exist_statement_.reset();

  if (db_) {
    if (sqlite3_close(db_) != SQLITE_OK) {
      const char* message = sqlite3_errmsg(db_);
      throw Exception{message};
    }
    db_ = nullptr;
  }
}

void connection::query(std::string_view sql) {
  assert(db_);
  if (sqlite3_exec(db_, std::string{sql}.c_str(), nullptr, nullptr, nullptr) !=
      SQLITE_OK) {
    const char* message = sqlite3_errmsg(db_);
    throw Exception{message};
  }
}

bool connection::table_exists(std::string_view table_name) const {
  if (!does_table_exist_statement_) {
    does_table_exist_statement_ = std::make_unique<statement>(
        *const_cast<connection*>(this),
        "SELECT name FROM sqlite_master WHERE type='table' AND name=?");
  }

  does_table_exist_statement_->bind(0, table_name);

  // Table exists if any row was returned.
  bool exists = does_table_exist_statement_->next();

  does_table_exist_statement_->reset();

  return exists;
}

bool connection::field_exists(std::string_view table_name,
                              std::string_view field_name) const {
  if (does_column_exist_table_name_ != table_name) {
    does_column_exist_table_name_ = table_name;
    does_column_exist_statement_ = std::make_unique<statement>(
        *const_cast<connection*>(this),
        std::format("PRAGMA TABLE_INFO({})", table_name));
  }

  bool exists = false;
  while (does_column_exist_statement_->next()) {
    if (does_column_exist_statement_->at(1).as_string().compare(field_name) ==
        0) {
      exists = true;
      break;
    }
  }

  does_column_exist_statement_->reset();

  return exists;
}

bool connection::index_exists(std::string_view table_name,
                              std::string_view index_name) const {
  if (does_index_exist_table_name_ != table_name) {
    does_index_exist_table_name_ = table_name;
    does_index_exist_statement_ = std::make_unique<statement>(
        *const_cast<connection*>(this),
        std::format("PRAGMA INDEX_LIST({})", table_name));
  }

  bool exists = false;
  while (does_index_exist_statement_->next()) {
    if (does_index_exist_statement_->at(1).as_string().compare(index_name) ==
        0) {
      exists = true;
      break;
    }
  }

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
  assert(db_);
  return sqlite3_changes(db_);
}

std::vector<field_info> connection::table_fields(
    std::string_view table_name) const {
  std::vector<field_info> fields;

  statement statement{*const_cast<connection*>(this),
                      std::format("PRAGMA TABLE_INFO({})", table_name)};

  while (statement.next()) {
    const auto& field_name = statement.at(1).as_string();
    const auto& field_type_string = statement.at(2).as_string();
    const auto field_type = parse_field_type(field_type_string);
    assert(field_type != field_type::EMPTY);
    fields.emplace_back(field_info{field_name, field_type});
  }

  return fields;
}

}  // namespace sql::sqlite3
