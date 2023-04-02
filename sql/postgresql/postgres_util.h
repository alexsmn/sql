#pragma once

#include "sql/exception.h"
#include "sql/types.h"

#include <libpq-fe.h>
#include <libpq/libpq-fs.h>

namespace sql::postgresql {

inline void CheckPostgresResult(const PGresult* result) {
  ExecStatusType status = PQresultStatus(result);
  if (status != PGRES_EMPTY_QUERY && status != PGRES_COMMAND_OK &&
      status != PGRES_TUPLES_OK && status != PGRES_SINGLE_TUPLE) {
    const char* message = PQresultErrorMessage(result);
    throw Exception{message};
  }
}

inline field_type ParsePostgresColumnType(std::string_view str) {
  static const std::pair<std::string_view, field_type> kNameMapping[] = {
      {"integer", field_type::INTEGER},
      {"smallint", field_type::INTEGER},
      {"bigint", field_type::INTEGER},
      {"float", field_type::FLOAT},
      {"double precision", field_type::FLOAT},
      {"text", field_type::TEXT}};
  auto i = std::ranges::find_if(kNameMapping,
                                [str](auto&& p) { return p.first == str; });
  return i != std::end(kNameMapping) ? i->second : field_type::EMPTY;
}

}  // namespace sql::postgresql
