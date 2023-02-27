#pragma once

#include <stdexcept>

namespace sql {

class Exception : public std::runtime_error {
 public:
  explicit Exception(const char* message) : runtime_error{message} {}
};

}  // namespace sql
