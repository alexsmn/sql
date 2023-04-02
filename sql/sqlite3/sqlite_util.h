#pragma once

#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <cassert>

namespace sql::sqlite3 {

inline constexpr std::pair<std::string_view, field_type>
    kSqliteColumnTypeNames[] = {
        {"INTEGER", field_type::INTEGER}, {"SMALLINT", field_type::INTEGER},
        {"BIGINT", field_type::INTEGER},  {"TEXT", field_type::TEXT},
        {"FLOAT", field_type::FLOAT},     {"REAL", field_type::FLOAT},
};

inline field_type ParseSqliteColumnType(std::string_view str) {
  auto i = std::ranges::find_if(kSqliteColumnTypeNames, [str](const auto& p) {
    return boost::algorithm::iequals(p.first, str);
  });
  return i != std::end(kSqliteColumnTypeNames) ? i->second : field_type::EMPTY;
}

}  // namespace sql::sqlite3
