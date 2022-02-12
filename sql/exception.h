#pragma once

#include <stdexcept>

namespace sql {

class Connection;

class Exception : public std::runtime_error {
 public:
  explicit Exception(Connection& connection);
};

} // namespace sql
