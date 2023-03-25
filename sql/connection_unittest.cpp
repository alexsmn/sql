#include "sql/connection.h"
#include "sql/statement.h"

#include <filesystem>
#include <gmock/gmock.h>

using namespace testing;

namespace sql {

class ConnectionTest : public TestWithParam<OpenParams> {
 public:
  virtual void TearDown() override;

 protected:
  Connection connection_;
  Statement statement_;

  std::filesystem::path temp_dir_;
};

INSTANTIATE_TEST_SUITE_P(
    ConnectionTests,
    ConnectionTest,
    Values(OpenParams{.driver = "sqlite"},
           OpenParams{
               .driver = "postgres",
               .connection_string = "host=localhost port=5433 dbname=test "
                                    "user=postgres password=1234"}));

void ConnectionTest::TearDown() {
  statement_.Close();
  connection_.Close();

  if (!temp_dir_.empty()) {
    std::filesystem::remove_all(temp_dir_);
  }
}

TEST_P(ConnectionTest, Test) {
  temp_dir_ = std::filesystem::temp_directory_path() / "sql";

  auto open_params = GetParam();
  open_params.path = temp_dir_;
  connection_.Open(open_params);

  if (connection_.DoesTableExist("test")) {
    connection_.Execute("DROP TABLE test");
  }

  EXPECT_FALSE(connection_.DoesTableExist("test"));

  connection_.Execute("CREATE TABLE test(A INTEGER, B INTEGER)");

  EXPECT_TRUE(connection_.DoesTableExist("test"));
  EXPECT_TRUE(connection_.DoesColumnExist("test", "A"));
  EXPECT_TRUE(connection_.DoesColumnExist("test", "B"));
  EXPECT_FALSE(connection_.DoesColumnExist("test", "C"));

  EXPECT_FALSE(connection_.DoesIndexExist("test", "A_Index"));

  connection_.Execute("CREATE INDEX A_Index ON test(A)");

  EXPECT_TRUE(connection_.DoesIndexExist("test", "A_Index"));
  EXPECT_FALSE(connection_.DoesIndexExist("test", "B_Index"));

  EXPECT_THAT(
      connection_.GetTableColumns("test"),
      UnorderedElementsAre(FieldsAre(StrCaseEq("A"), COLUMN_TYPE_INTEGER),
                           FieldsAre(StrCaseEq("B"), COLUMN_TYPE_INTEGER)));

  Statement insert_statement;
  insert_statement.Init(connection_, "INSERT INTO test VALUES($1, $2)");
  for (int i = 1; i <= 3; ++i) {
    insert_statement.Bind(0, i * 10);
    insert_statement.Bind(1, i * 100);
    insert_statement.Run();
  }

  std::vector<std::pair<int, int>> values;
  statement_.Init(connection_, "SELECT * FROM test");
  while (statement_.Step()) {
    int a = statement_.GetColumnInt(0);
    int b = statement_.GetColumnInt(1);
    values.emplace_back(a, b);
  }

  EXPECT_THAT(values, ElementsAre(FieldsAre(10, 100), FieldsAre(20, 200),
                                  FieldsAre(30, 300)));

  connection_.Execute("DROP TABLE test");
}

}  // namespace sql
