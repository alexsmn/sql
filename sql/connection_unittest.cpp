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

  connection_.Execute("CREATE TABLE test(A INTEGER, B BIGINT, C TEXT)");

  EXPECT_TRUE(connection_.DoesTableExist("test"));
  EXPECT_TRUE(connection_.DoesColumnExist("test", "A"));
  EXPECT_TRUE(connection_.DoesColumnExist("test", "B"));
  EXPECT_TRUE(connection_.DoesColumnExist("test", "C"));
  EXPECT_FALSE(connection_.DoesColumnExist("test", "D"));

  EXPECT_FALSE(connection_.DoesIndexExist("test", "A_Index"));

  connection_.Execute("CREATE INDEX A_Index ON test(A)");

  EXPECT_TRUE(connection_.DoesIndexExist("test", "A_Index"));
  EXPECT_FALSE(connection_.DoesIndexExist("test", "B_Index"));

  EXPECT_THAT(
      connection_.GetTableColumns("test"),
      UnorderedElementsAre(FieldsAre(StrCaseEq("A"), COLUMN_TYPE_INTEGER),
                           FieldsAre(StrCaseEq("B"), COLUMN_TYPE_INTEGER),
                           FieldsAre(StrCaseEq("C"), COLUMN_TYPE_TEXT)));

  Statement insert_statement;
  insert_statement.Init(connection_, "INSERT INTO test VALUES(?, ?, ?)");
  for (int i = 1; i <= 3; ++i) {
    insert_statement.Bind(0, i * 10);
    insert_statement.Bind(1, i * 100);
    insert_statement.Bind(2, std::string{3, static_cast<char>('A' + i)});
    insert_statement.Run();
    EXPECT_EQ(1, connection_.GetLastChangeCount());
    insert_statement.Reset();
  }

  struct Row {
    int a;
    int64_t b;
    std::string c;
  };

  std::vector<Row> rows;
  statement_.Init(connection_, "SELECT * FROM test");
  while (statement_.Step()) {
    EXPECT_EQ(COLUMN_TYPE_INTEGER, statement_.GetColumnType(0));
    EXPECT_EQ(COLUMN_TYPE_INTEGER, statement_.GetColumnType(1));
    EXPECT_EQ(COLUMN_TYPE_TEXT, statement_.GetColumnType(2));
    auto a = statement_.GetColumnInt(0);
    auto b = statement_.GetColumnInt64(1);
    auto c = statement_.GetColumnString(1);
    rows.emplace_back(a, b, std::move(c));
  }

  EXPECT_THAT(rows,
              ElementsAre(FieldsAre(10, 100, "AAA"), FieldsAre(20, 200, "BBB"),
                          FieldsAre(30, 300, "CCC")));

  connection_.Execute("DROP TABLE test");
}

}  // namespace sql
