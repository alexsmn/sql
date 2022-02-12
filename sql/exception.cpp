#include "sql/exception.h"

#include "sql/connection.h"

#include <sqlite3.h>

namespace sql {

Exception::Exception(Connection& connection)
    : std::runtime_error(sqlite3_errmsg(connection.db_)) {
}

} // namespace sql
