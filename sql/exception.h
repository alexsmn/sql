#pragma once

#include <stdexcept>

namespace sql {

class Exception : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

}  // namespace sql
