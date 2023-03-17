#include "sql/sqlite3/statement.h"

#include "sql/exception.h"
#include "sql/sqlite3/connection.h"

#include <boost/locale/encoding_utf.hpp>
#include <cassert>
#include <sqlite3.h>

namespace sql::sqlite3 {

Statement::Statement() {}

Statement::~Statement() {
  Close();
}

void Statement::Init(Connection& connection, std::string_view sql) {
  assert(connection.db_);

  int error = sqlite3_prepare_v2(connection.db_, sql.data(),
                                 static_cast<int>(sql.size()), &stmt_, NULL);
  if (error != SQLITE_OK) {
    stmt_ = NULL;
    throw Exception{sqlite3_errmsg(connection.db_)};
  }

  connection_ = &connection;
}

void Statement::BindNull(unsigned column) {
  if (sqlite3_bind_null(stmt_, column + 1) != SQLITE_OK)
    throw Exception{sqlite3_errmsg(connection_->db_)};
}

void Statement::Bind(unsigned column, bool value) {
  Bind(column, value ? 1 : 0);
}

void Statement::Bind(unsigned column, int value) {
  assert(stmt_);
  if (sqlite3_bind_int(stmt_, column + 1, value) != SQLITE_OK)
    throw Exception{sqlite3_errmsg(connection_->db_)};
}

void Statement::Bind(unsigned column, int64_t value) {
  assert(stmt_);
  if (sqlite3_bind_int64(stmt_, column + 1, value) != SQLITE_OK)
    throw Exception{sqlite3_errmsg(connection_->db_)};
}

void Statement::Bind(unsigned column, double value) {
  assert(stmt_);
  if (sqlite3_bind_double(stmt_, column + 1, value) != SQLITE_OK)
    throw Exception{sqlite3_errmsg(connection_->db_)};
}

void Statement::Bind(unsigned column, std::string_view value) {
  assert(stmt_);
  if (sqlite3_bind_text(stmt_, column + 1, value.data(),
                        static_cast<int>(value.size()),
                        SQLITE_TRANSIENT) != SQLITE_OK)
    throw Exception{sqlite3_errmsg(connection_->db_)};
}

void Statement::Bind(unsigned column, std::u16string_view value) {
  return Bind(column, boost::locale::conv::utf_to_utf<char>(
                          value.data(), value.data() + value.size()));
}

size_t Statement::GetColumnCount() const {
  assert(stmt_);
  return sqlite3_column_count(stmt_);
}

ColumnType Statement::GetColumnType(unsigned column) const {
  // Verify that our enum matches sqlite's values.
  static_assert(COLUMN_TYPE_INTEGER == SQLITE_INTEGER, "BadIntegerType");
  static_assert(COLUMN_TYPE_FLOAT == SQLITE_FLOAT, "BadFloatType");
  static_assert(COLUMN_TYPE_TEXT == SQLITE_TEXT, "BadTextType");
  static_assert(COLUMN_TYPE_BLOB == SQLITE_BLOB, "BadBlobType");
  static_assert(COLUMN_TYPE_NULL == SQLITE_NULL, "BadNullType");

  assert(stmt_);
  return static_cast<ColumnType>(sqlite3_column_type(stmt_, column));
}

bool Statement::GetColumnBool(unsigned column) const {
  return GetColumnInt(column) != 0;
}

int Statement::GetColumnInt(unsigned column) const {
  return sqlite3_column_int(stmt_, column);
}

int64_t Statement::GetColumnInt64(unsigned column) const {
  return sqlite3_column_int64(stmt_, column);
}

double Statement::GetColumnDouble(unsigned column) const {
  return sqlite3_column_double(stmt_, column);
}

std::string Statement::GetColumnString(unsigned column) const {
  const char* text =
      reinterpret_cast<const char*>(sqlite3_column_text(stmt_, column));
  int length = sqlite3_column_bytes(stmt_, column);

  if (text && length > 0)
    return std::string(text, static_cast<size_t>(length));
  else
    return std::string();
}

std::u16string Statement::GetColumnString16(unsigned column) const {
  std::string string = GetColumnString(column);
  return string.empty() ? std::u16string()
                        : boost::locale::conv::utf_to_utf<char16_t>(string);
}

void Statement::Run() {
  assert(stmt_);
  int result = sqlite3_step(stmt_);
  if (result != SQLITE_DONE)
    throw Exception{sqlite3_errmsg(connection_->db_)};
}

bool Statement::Step() {
  assert(stmt_);
  int result = sqlite3_step(stmt_);
  if (result == SQLITE_ROW)
    return true;
  if (result == SQLITE_DONE)
    return false;
  throw Exception{sqlite3_errmsg(connection_->db_)};
}

void Statement::Reset() {
  assert(stmt_);
  sqlite3_clear_bindings(stmt_);
  sqlite3_reset(stmt_);
}

void Statement::Close() {
  if (stmt_) {
    sqlite3_finalize(stmt_);
    stmt_ = NULL;
  }
}

}  // namespace sql::sqlite3
