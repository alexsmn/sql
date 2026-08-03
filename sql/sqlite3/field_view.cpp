#include "sql/sqlite3/field_view.h"

#include <boost/locale/encoding_utf.hpp>
#include <cassert>
#include <sqlite3.h>

namespace sql::sqlite3 {

field_view::field_view(::sqlite3_stmt* stmt, int field_index)
    : stmt_{stmt}, field_index_{field_index} {}

field_type field_view::type() const {
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
  return static_cast<sql::field_type>(sqlite3_column_type(stmt_, field_index_));
}

bool field_view::as_bool() const {
  return as_int() != 0;
}

int field_view::as_int() const {
  return sqlite3_column_int(stmt_, field_index_);
}

int64_t field_view::as_int64() const {
  return sqlite3_column_int64(stmt_, field_index_);
}

double field_view::as_double() const {
  return sqlite3_column_double(stmt_, field_index_);
}

std::string_view field_view::as_string_view() const {
  const char* text =
      reinterpret_cast<const char*>(sqlite3_column_text(stmt_, field_index_));
  int length = sqlite3_column_bytes(stmt_, field_index_);

  if (text && length > 0)
    return std::string_view{text, static_cast<size_t>(length)};
  else
    return std::string_view{};
}

std::string field_view::as_string() const {
  return std::string{as_string_view()};
}

std::u16string field_view::as_string16() const {
  std::string string = as_string();
  return string.empty() ? std::u16string()
                        : boost::locale::conv::utf_to_utf<char16_t>(string);
}

}  // namespace sql::sqlite3
