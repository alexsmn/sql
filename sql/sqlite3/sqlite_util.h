#pragma once

#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <cassert>

namespace sql::sqlite3 {

inline field_type parse_field_type(std::string_view str) {
  constexpr std::pair<std::string_view, field_type> mapping[] = {
      {"INTEGER", field_type::INTEGER}, {"SMALLINT", field_type::INTEGER},
      {"BIGINT", field_type::INTEGER},  {"TEXT", field_type::TEXT},
      {"FLOAT", field_type::FLOAT},     {"REAL", field_type::FLOAT},
  };

  auto i = std::ranges::find_if(mapping, [str](const auto& p) {
    return boost::algorithm::iequals(p.first, str);
  });
  return i != std::end(mapping) ? i->second : field_type::EMPTY;
}

}  // namespace sql::sqlite3
