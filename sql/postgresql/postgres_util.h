#pragma once

#include "sql/exception.h"
#include "sql/types.h"

#include <libpq-fe.h>
#include <libpq/libpq-fs.h>

namespace sql::postgresql {

inline void CheckPostgresResult(PGresult* result) {
  ExecStatusType status = PQresultStatus(result);
  if (status != PGRES_EMPTY_QUERY && status != PGRES_COMMAND_OK &&
      status != PGRES_TUPLES_OK && status != PGRES_SINGLE_TUPLE) {
    throw Exception{PQresultErrorMessage(result)};
  }
}

inline ColumnType ParsePostgresColumnType(std::string_view str) {
  static const std::pair<std::string_view, ColumnType> kNameMapping[] = {
      {"int", ColumnType::COLUMN_TYPE_INTEGER},
      {"float", ColumnType::COLUMN_TYPE_FLOAT},
      {"text", ColumnType::COLUMN_TYPE_TEXT}};
  auto i = std::ranges::find_if(kNameMapping,
                                [str](auto&& p) { return p.first == str; });
  return i != std::end(kNameMapping) ? i->second : ColumnType::COLUMN_TYPE_NULL;
}

}  // namespace sql::postgresql
