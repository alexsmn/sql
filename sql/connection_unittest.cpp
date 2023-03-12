#include <gmock/gmock.h>

#include "sql/connection.h"
#include "sql/statement.h"

using namespace testing;

namespace sql {

class ConnectionTest : public TestWithParam<std::string> {
 protected:
  Connection connection_;
  Statement statement_;
};

}  // namespace sql
