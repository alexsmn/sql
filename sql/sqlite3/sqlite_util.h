#pragma once

#include <algorithm>
#include <cassert>

namespace sql::sqlite3 {

inline constexpr std::pair<std::string_view, ColumnType>
    kSqliteColumnTypeNames[] = {
        {"INTEGER", COLUMN_TYPE_INTEGER}, {"SMALLINT", COLUMN_TYPE_INTEGER},
        {"BIGINT", COLUMN_TYPE_INTEGER},  {"TEXT", COLUMN_TYPE_TEXT},
        {"FLOAT", COLUMN_TYPE_FLOAT},     {"REAL", COLUMN_TYPE_FLOAT},
};

inline ColumnType ParseSqliteColumnType(std::string_view str) {
  auto i = std::find_if(std::begin(kSqliteColumnTypeNames),
                        std::end(kSqliteColumnTypeNames),
                        [str](const auto& p) { return p.first == str; });
  return i != std::end(kSqliteColumnTypeNames) ? i->second : COLUMN_TYPE_NULL;
}

}  // namespace sql::sqlite3
