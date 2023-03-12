#include "sql/connection.h"
#include "sql/statement.h"

#include <filesystem>
#include <gmock/gmock.h>

using namespace testing;

namespace sql {

class ConnectionTest : public TestWithParam<std::string> {
 public:
  virtual void TearDown() override;

 protected:
  Connection connection_;
  Statement statement_;

  std::filesystem::path temp_dir_;
};

INSTANTIATE_TEST_CASE_P(ConnectionTests, ConnectionTest, Values("sqlite"));

void ConnectionTest::TearDown() {
  statement_.Close();
  connection_.Close();

  if (!temp_dir_.empty()) {
    std::filesystem::remove_all(temp_dir_);
  }
}

TEST_P(ConnectionTest, Test) {
  temp_dir_ = std::filesystem::temp_directory_path() / "sql";

  connection_.Open({.driver = GetParam(), .path = temp_dir_});

  connection_.Execute("CREATE TABLE test(A INTEGER, B INTEGER)");

  statement_.Init(connection_, "SELECT * FROM test");
}

}  // namespace sql
