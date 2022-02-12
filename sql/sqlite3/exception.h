#pragma once

#include "sql/exception.h"

namespace sql::sqlite3 {

class Connection;

class Exception : public sql::Exception {
 public:
  explicit Exception(Connection& connection);
};

}  // namespace sql::sqlite3
