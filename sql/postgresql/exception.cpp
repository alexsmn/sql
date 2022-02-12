#include "sql/postgresql/exception.h"

#include "sql/postgresql/connection.h"

#include <sqlite3.h>

namespace sql::postgresql {

Exception::Exception(Connection& connection)
    : sql::Exception{sqlite3_errmsg(connection.db_)} {}

}  // namespace sql::postgresql
