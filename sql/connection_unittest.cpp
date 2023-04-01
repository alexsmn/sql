#include "sql/connection.h"
#include "sql/postgresql/connection.h"
#include "sql/postgresql/statement.h"
#include "sql/sqlite3/connection.h"
#include "sql/sqlite3/statement.h"
#include "sql/statement.h"
#include "sql/test/temp_dir.h"

#include <filesystem>
#include <gmock/gmock.h>

using namespace testing;

namespace sql {

template <class T>
struct ConnectionTraits;

template <>
struct ConnectionTraits<sql::sqlite3::Connection> {
  sql::OpenParams GetOpenParams() {
    return {.driver = "sqlite", .path = temp_dir_.get()};
  }

  ScopedTempDir temp_dir_;
};

template <>
struct ConnectionTraits<sql::postgresql::Connection> {
  sql::OpenParams GetOpenParams() {
    return {
        .driver = "postgres",
        .connection_string =
            "host=localhost port=5433 dbname=test user=postgres password=1234"};
  }
};

template <>
struct ConnectionTraits<sql::Connection>
    : ConnectionTraits<sql::sqlite3::Connection> {};

template <class T>
class ConnectionTest : public Test {
 public:
  using StatementType = typename T::Statement;

  virtual void SetUp() override;
  virtual void TearDown() override;

 protected:
  T connection_;
  ConnectionTraits<T> connection_traits_;
};

using ConnectionTypes = ::testing::Types<sql::Connection,
                                         sql::sqlite3::Connection,
                                         sql::postgresql::Connection>;
TYPED_TEST_SUITE(ConnectionTest, ConnectionTypes);

template <class T>
void ConnectionTest<T>::SetUp() {
  this->connection_.Open(this->connection_traits_.GetOpenParams());

  if (this->connection_.DoesTableExist("test")) {
    this->connection_.Execute("DROP TABLE test");
  }

  EXPECT_FALSE(this->connection_.DoesTableExist("test"));

  this->connection_.Execute("CREATE TABLE test(A INTEGER, B BIGINT, C TEXT)");
}

template <class T>
void ConnectionTest<T>::TearDown() {
  if (this->connection_.DoesTableExist("test")) {
    this->connection_.Execute("DROP TABLE test");
  }

  connection_.Close();
}

TYPED_TEST(ConnectionTest, TestColumns) {
  EXPECT_TRUE(this->connection_.DoesTableExist("test"));
  EXPECT_TRUE(this->connection_.DoesColumnExist("test", "A"));
  EXPECT_TRUE(this->connection_.DoesColumnExist("test", "B"));
  EXPECT_TRUE(this->connection_.DoesColumnExist("test", "C"));
  EXPECT_FALSE(this->connection_.DoesColumnExist("test", "D"));

  EXPECT_THAT(
      this->connection_.GetTableColumns("test"),
      UnorderedElementsAre(FieldsAre(StrCaseEq("A"), COLUMN_TYPE_INTEGER),
                           FieldsAre(StrCaseEq("B"), COLUMN_TYPE_INTEGER),
                           FieldsAre(StrCaseEq("C"), COLUMN_TYPE_TEXT)));
}

TYPED_TEST(ConnectionTest, TestIndexes) {
  EXPECT_FALSE(this->connection_.DoesIndexExist("test", "A_Index"));

  this->connection_.Execute("CREATE INDEX A_Index ON test(A)");

  EXPECT_TRUE(this->connection_.DoesIndexExist("test", "A_Index"));
  EXPECT_FALSE(this->connection_.DoesIndexExist("test", "B_Index"));
}

TYPED_TEST(ConnectionTest, TestStatements) {
  using ConnectionType = TypeParam;
  using StatementType = ConnectionType::Statement;

  StatementType insert_statement{this->connection_,
                                 "INSERT INTO test VALUES(?, ?, ?)"};
  for (int i = 1; i <= 3; ++i) {
    insert_statement.Bind(0, i * 10);
    insert_statement.Bind(1, i * 100);
    insert_statement.Bind(2, std::string(3, static_cast<char>('A' + i - 1)));
    insert_statement.Run();
    EXPECT_EQ(1, this->connection_.GetLastChangeCount());
    insert_statement.Reset();
  }

  struct Row {
    int a;
    int64_t b;
    std::string c;
  };

  std::vector<Row> rows;

  StatementType statement{this->connection_, "SELECT * FROM test"};
  while (statement.Step()) {
    EXPECT_EQ(COLUMN_TYPE_INTEGER, statement.GetColumnType(0));
    EXPECT_EQ(COLUMN_TYPE_INTEGER, statement.GetColumnType(1));
    EXPECT_EQ(COLUMN_TYPE_TEXT, statement.GetColumnType(2));
    auto a = statement.GetColumnInt(0);
    auto b = statement.GetColumnInt64(1);
    auto c = statement.GetColumnString(2);
    rows.emplace_back(a, b, std::move(c));
  }

  EXPECT_THAT(rows,
              ElementsAre(FieldsAre(10, 100, "AAA"), FieldsAre(20, 200, "BBB"),
                          FieldsAre(30, 300, "CCC")));
}

}  // namespace sql
