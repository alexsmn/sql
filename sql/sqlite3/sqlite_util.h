#pragma once

#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <cassert>

namespace sql::sqlite3 {

inline constexpr std::pair<std::string_view, ColumnType>
    kSqliteColumnTypeNames[] = {
        {"INTEGER", COLUMN_TYPE_INTEGER}, {"SMALLINT", COLUMN_TYPE_INTEGER},
        {"BIGINT", COLUMN_TYPE_INTEGER},  {"TEXT", COLUMN_TYPE_TEXT},
        {"FLOAT", COLUMN_TYPE_FLOAT},     {"REAL", COLUMN_TYPE_FLOAT},
};

inline ColumnType ParseSqliteColumnType(std::string_view str) {
  auto i = std::ranges::find_if(kSqliteColumnTypeNames, [str](const auto& p) {
    return boost::algorithm::iequals(p.first, str);
  });
  return i != std::end(kSqliteColumnTypeNames) ? i->second : COLUMN_TYPE_NULL;
}

}  // namespace sql::sqlite3
