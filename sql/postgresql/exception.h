#pragma once

#include "sql/exception.h"

namespace sql::postgresql {

class Connection;

class Exception : public sql::Exception {
 public:
  explicit Exception(Connection& connection);
};

}  // namespace sql::postgresql
