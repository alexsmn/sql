#include "sql/sqlite3/statement.h"

#include "base/strings/utf_string_conversions.h"
#include "sql/sqlite3/connection.h"
#include "sql/sqlite3/exception.h"

#include <cassert>
#include <sqlite3.h>

namespace sql::sqlite3 {

Statement::Statement() : connection_(NULL), stmt_(NULL) {}

Statement::~Statement() {
  Close();
}

Statement::Statement(Statement&& source)
    : connection_{source.connection_}, stmt_{source.stmt_} {
  source.connection_ = nullptr;
  source.stmt_ = nullptr;
}

Statement& Statement::operator=(Statement&& source) {
  if (this != &source) {
    connection_ = source.connection_;
    stmt_ = source.stmt_;
    source.connection_ = nullptr;
    source.stmt_ = nullptr;
  }
  return *this;
}

void Statement::Init(Connection& connection, const char* sql) {
  assert(connection.db_);

  int error = sqlite3_prepare_v2(connection.db_, sql, -1, &stmt_, NULL);
  if (error != SQLITE_OK) {
    stmt_ = NULL;
    throw Exception(connection);
  }

  connection_ = &connection;
}

void Statement::BindNull(unsigned column) {
  if (sqlite3_bind_null(stmt_, column + 1) != SQLITE_OK)
    throw Exception(*connection_);
}

void Statement::Bind(unsigned column, bool value) {
  Bind(column, value ? 1 : 0);
}

void Statement::Bind(unsigned column, int value) {
  assert(stmt_);
  if (sqlite3_bind_int(stmt_, column + 1, value) != SQLITE_OK)
    throw Exception(*connection_);
}

void Statement::Bind(unsigned column, int64_t value) {
  assert(stmt_);
  if (sqlite3_bind_int64(stmt_, column + 1, value) != SQLITE_OK)
    throw Exception(*connection_);
}

void Statement::Bind(unsigned column, double value) {
  assert(stmt_);
  if (sqlite3_bind_double(stmt_, column + 1, value) != SQLITE_OK)
    throw Exception(*connection_);
}

void Statement::Bind(unsigned column, const char* value) {
  assert(stmt_);
  if (sqlite3_bind_text(stmt_, column + 1, value, -1, SQLITE_TRANSIENT) !=
      SQLITE_OK)
    throw Exception(*connection_);
}

void Statement::Bind(unsigned column, const std::string& value) {
  assert(stmt_);
  if (sqlite3_bind_text(stmt_, column + 1, value.data(), value.length(),
                        SQLITE_TRANSIENT) != SQLITE_OK)
    throw Exception(*connection_);
}

void Statement::Bind(unsigned column, const std::wstring& value) {
  return Bind(column, base::UTF16ToUTF8(value));
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

std::wstring Statement::GetColumnString16(unsigned column) const {
  std::string string = GetColumnString(column);
  return string.empty() ? std::wstring() : base::UTF8ToUTF16(string);
}

void Statement::Run() {
  assert(stmt_);
  int result = sqlite3_step(stmt_);
  if (result != SQLITE_DONE)
    throw Exception(*connection_);
}

bool Statement::Step() {
  assert(stmt_);
  int result = sqlite3_step(stmt_);
  if (result == SQLITE_ROW)
    return true;
  if (result == SQLITE_DONE)
    return false;
  throw Exception(*connection_);
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