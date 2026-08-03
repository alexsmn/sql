#pragma once

#include <sql/statement.h>

namespace sql {

// TODO: Make templated and rename to `scoped_reset`.
class scoped_reset_statement {
 public:
  explicit scoped_reset_statement(statement& stmt) : stmt_{stmt} {}

  ~scoped_reset_statement() { stmt_.reset(); }

 private:
  statement& stmt_;
};

}  // namespace sql