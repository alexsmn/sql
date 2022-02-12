#include "sql/sqlite3/exception.h"

#include "sql/sqlite3/connection.h"

#include <sqlite3.h>

namespace sql::sqlite3 {

Exception::Exception(Connection& connection)
    : sql::Exception{sqlite3_errmsg(connection.db_)} {}

}  // namespace sql::sqlite3
