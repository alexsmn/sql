#pragma once

#include <algorithm>
#include <cassert>

namespace sql::sqlite3 {

inline constexpr std::pair<std::string_view, ColumnType>
    kSqliteColumnTypeNames[] = {
        {"INTEGER", COLUMN_TYPE_INTEGER}, {"smallint", COLUMN_TYPE_TEXT},
        {"bigint", COLUMN_TYPE_TEXT},     {"TEXT", COLUMN_TYPE_TEXT},
        {"REAL", COLUMN_TYPE_FLOAT},
};

inline ColumnType ParseSqliteColumnType(std::string_view str) {
  auto i = std::find_if(std::begin(kSqliteColumnTypeNames),
                        std::end(kSqliteColumnTypeNames),
                        [str](const auto& p) { return p.first == str; });
  return i != std::end(kSqliteColumnTypeNames) ? i->second : COLUMN_TYPE_NULL;
}

}  // namespace sql::sqlite3