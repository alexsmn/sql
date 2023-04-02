#include "sql/sqlite3/statement.h"

#include "sql/exception.h"
#include "sql/sqlite3/connection.h"

#include <boost/locale/encoding_utf.hpp>
#include <cassert>
#include <sqlite3.h>

namespace sql::sqlite3 {

namespace {

void CheckSqliteResult(::sqlite3* db, int result) {
  assert(db != nullptr);
  if (result != SQLITE_OK) {
    const char* message = sqlite3_errmsg(db);
    throw Exception{message};
  }
}

}  // namespace

// statement

statement::statement(connection& connection, std::string_view sql) {
  prepare(connection, sql);
}

statement::~statement() {
  close();
}

void statement::prepare(connection& connection, std::string_view sql) {
  assert(connection.db_);

  int error = sqlite3_prepare_v2(connection.db_, sql.data(),
                                 static_cast<int>(sql.size()), &stmt_, NULL);
  if (error != SQLITE_OK) {
    stmt_ = NULL;
    const char* message = sqlite3_errmsg(connection.db_);
    throw Exception{message};
  }

  connection_ = &connection;
}

void statement::bind_null(unsigned column) {
  assert(stmt_);
  CheckSqliteResult(connection_->db_, sqlite3_bind_null(stmt_, column + 1));
}

void statement::bind(unsigned column, bool value) {
  bind(column, value ? 1 : 0);
}

void statement::bind(unsigned column, int value) {
  assert(stmt_);
  CheckSqliteResult(connection_->db_,
                    sqlite3_bind_int(stmt_, column + 1, value));
}

void statement::bind(unsigned column, int64_t value) {
  assert(stmt_);
  CheckSqliteResult(connection_->db_,
                    sqlite3_bind_int64(stmt_, column + 1, value));
}

void statement::bind(unsigned column, double value) {
  assert(stmt_);
  CheckSqliteResult(connection_->db_,
                    sqlite3_bind_double(stmt_, column + 1, value));
}

void statement::bind(unsigned column, const char* value) {
  bind(column, std::string_view{value});
}

void statement::bind(unsigned column, const char16_t* value) {
  bind(column, std::u16string_view{value});
}

void statement::bind(unsigned column, std::string_view value) {
  assert(stmt_);
  CheckSqliteResult(
      connection_->db_,
      sqlite3_bind_text(stmt_, column + 1, value.data(),
                        static_cast<int>(value.size()), SQLITE_TRANSIENT));
}

void statement::bind(unsigned column, std::u16string_view value) {
  bind(column, boost::locale::conv::utf_to_utf<char>(
                   value.data(), value.data() + value.size()));
}

size_t statement::field_count() const {
  assert(stmt_);
  return sqlite3_column_count(stmt_);
}

field_type statement::field_type(unsigned column) const {
  // Verify that our enum matches sqlite's values.
  static_assert(static_cast<int>(sql::field_type::INTEGER) == SQLITE_INTEGER,
                "BadIntegerType");
  static_assert(static_cast<int>(sql::field_type::FLOAT) == SQLITE_FLOAT,
                "BadFloatType");
  static_assert(static_cast<int>(sql::field_type::TEXT) == SQLITE_TEXT,
                "BadTextType");
  static_assert(static_cast<int>(sql::field_type::BLOB) == SQLITE_BLOB,
                "BadBlobType");
  static_assert(static_cast<int>(sql::field_type::EMPTY) == SQLITE_NULL,
                "BadNullType");

  assert(stmt_);
  return static_cast<sql::field_type>(sqlite3_column_type(stmt_, column));
}

field_view statement::at(unsigned column) const {
  return field_view{stmt_, static_cast<int>(column)};
}

void statement::query() {
  assert(stmt_);
  int result = sqlite3_step(stmt_);
  if (result != SQLITE_DONE) {
    const char* message = sqlite3_errmsg(connection_->db_);
    throw Exception{message};
  }
}

bool statement::next() {
  assert(stmt_);
  int result = sqlite3_step(stmt_);
  if (result == SQLITE_ROW)
    return true;
  if (result == SQLITE_DONE)
    return false;
  const char* message = sqlite3_errmsg(connection_->db_);
  throw Exception{message};
}

void statement::reset() {
  assert(stmt_);
  sqlite3_clear_bindings(stmt_);
  sqlite3_reset(stmt_);
}

void statement::close() {
  if (stmt_) {
    sqlite3_finalize(stmt_);
    stmt_ = NULL;
  }
}

}  // namespace sql::sqlite3
