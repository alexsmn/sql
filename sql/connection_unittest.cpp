#include "sql/connection.h"
#include "sql/postgresql/connection.h"
#include "sql/postgresql/statement.h"
#include "sql/sqlite3/connection.h"
#include "sql/sqlite3/statement.h"
#include "sql/statement.h"
#include "sql/test/temp_dir.h"

#include <filesystem>
#include <format>
#include <gmock/gmock.h>
#include <random>
#include <span>

using namespace testing;

namespace sql {

template <class T>
struct ConnectionTraits;

template <>
struct ConnectionTraits<sql::sqlite3::Connection> {
  sql::OpenParams GetOpenParams() {
    return {.driver = "sqlite", .path = temp_dir_.get() / "database.sqlite3"};
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

struct Row {
  int a;
  int64_t b;
  std::string c;

  friend auto operator<=>(const Row&, const Row&) = default;

  friend std::ostream& operator<<(std::ostream& stream, const Row& row) {
    return stream << std::format("(.a={}, .b={}, .c={})", row.a, row.b, row.c);
  }
};

template <class T>
class ConnectionTest : public Test {
 public:
  using StatementType = typename T::Statement;

  virtual void SetUp() override;
  virtual void TearDown() override;

 protected:
  void InsertTestData(std::span<const Row> rows);

  T connection_;
  ConnectionTraits<T> connection_traits_;

  std::string table_name_;
};

using ConnectionTypes = ::testing::Types<sql::Connection,
                                         sql::sqlite3::Connection,
                                         sql::postgresql::Connection>;
TYPED_TEST_SUITE(ConnectionTest, ConnectionTypes);

std::vector<Row> GenerateRows() {
  std::vector<Row> rows;
  for (int i = 1; i <= 3; ++i) {
    rows.push_back(
        Row{.a = i * 10, .b = i * 100, .c = static_cast<char>('A' + i - 1)});
  }
  return rows;
}

template <class T>
std::string GetTempTableName(const T& connection) {
  std::mt19937 gen{std::random_device{}()};
  std::uniform_int_distribution<> distrib(0, std::numeric_limits<int>::max());

  for (int i = 0; i < 15; ++i) {
    auto table_name = std::format("test_{}", distrib(gen));
    if (!connection.DoesTableExist(table_name)) {
      return table_name;
    }
  }
  throw std::runtime_error{"Cannot create a temp table"};
}

template <class T>
void ConnectionTest<T>::SetUp() {
  this->connection_.Open(this->connection_traits_.GetOpenParams());

  table_name_ = GetTempTableName(this->connection_);

  this->connection_.Execute(
      std::format("CREATE TABLE {}(A INTEGER, B BIGINT, C TEXT)", table_name_));
}

template <class T>
void ConnectionTest<T>::TearDown() {
  if (this->connection_.DoesTableExist(table_name_)) {
    this->connection_.Execute(std::format("DROP TABLE {}", table_name_));
  }

  connection_.Close();
}

TYPED_TEST(ConnectionTest, TestColumns) {
  const auto& table_name = this->table_name_;

  EXPECT_TRUE(this->connection_.DoesTableExist(table_name));
  EXPECT_TRUE(this->connection_.DoesColumnExist(table_name, "A"));
  EXPECT_TRUE(this->connection_.DoesColumnExist(table_name, "B"));
  EXPECT_TRUE(this->connection_.DoesColumnExist(table_name, "C"));
  EXPECT_FALSE(this->connection_.DoesColumnExist(table_name, "D"));

  EXPECT_THAT(
      this->connection_.GetTableColumns(table_name),
      UnorderedElementsAre(FieldsAre(StrCaseEq("A"), COLUMN_TYPE_INTEGER),
                           FieldsAre(StrCaseEq("B"), COLUMN_TYPE_INTEGER),
                           FieldsAre(StrCaseEq("C"), COLUMN_TYPE_TEXT)));
}

TYPED_TEST(ConnectionTest, TestIndexes) {
  const auto& table_name = this->table_name_;

  EXPECT_FALSE(this->connection_.DoesIndexExist(table_name, "A_Index"));

  this->connection_.Execute(
      std::format("CREATE INDEX A_Index ON {}(A)", table_name));

  EXPECT_TRUE(this->connection_.DoesIndexExist(table_name, "A_Index"));
  EXPECT_FALSE(this->connection_.DoesIndexExist(table_name, "B_Index"));
}

template <class T>
void ConnectionTest<T>::InsertTestData(std::span<const Row> rows) {
  StatementType insert_statement{
      connection_, std::format("INSERT INTO {} VALUES(?, ?, ?)", table_name_)};

  for (auto& row : rows) {
    insert_statement.Bind(0, row.a);
    insert_statement.Bind(1, row.b);
    insert_statement.Bind(2, row.c);
    insert_statement.Run();
    EXPECT_EQ(1, connection_.GetLastChangeCount());
    insert_statement.Reset();
  }
}

template <class T>
Row ReadRow(T& statement) {
  std::vector<Row> rows;

  while (statement.Step()) {
    EXPECT_EQ(COLUMN_TYPE_INTEGER, statement.GetColumnType(0));
    EXPECT_EQ(COLUMN_TYPE_INTEGER, statement.GetColumnType(1));
    EXPECT_EQ(COLUMN_TYPE_TEXT, statement.GetColumnType(2));
    auto a = statement.GetColumnInt(0);
    auto b = statement.GetColumnInt64(1);
    auto c = statement.GetColumnString(2);
    rows.emplace_back(a, b, std::move(c));
  }

  statement.Reset();

  return rows;
}

template <class T>
std::vector<Row> ReadAllRows(T& statement) {
  std::vector<Row> rows;

  while (statement.Step()) {
    EXPECT_EQ(COLUMN_TYPE_INTEGER, statement.GetColumnType(0));
    EXPECT_EQ(COLUMN_TYPE_INTEGER, statement.GetColumnType(1));
    EXPECT_EQ(COLUMN_TYPE_TEXT, statement.GetColumnType(2));
    auto a = statement.GetColumnInt(0);
    auto b = statement.GetColumnInt64(1);
    auto c = statement.GetColumnString(2);
    rows.emplace_back(a, b, std::move(c));
  }

  statement.Reset();

  return rows;
}

TYPED_TEST(ConnectionTest, TestStatements) {
  const auto& table_name = this->table_name_;

  auto initial_rows = GenerateRows();
  this->InsertTestData(initial_rows);

  using ConnectionType = TypeParam;
  using StatementType = ConnectionType::Statement;

  StatementType statement{this->connection_,
                          std::format("SELECT * FROM {}", table_name)};
  auto rows = ReadAllRows(statement);

  EXPECT_THAT(rows, ElementsAreArray(initial_rows));
}

TYPED_TEST(ConnectionTest, ParametrizedStatement) {
  const auto& table_name = this->table_name_;

  auto initial_rows = GenerateRows();
  this->InsertTestData(initial_rows);

  using ConnectionType = TypeParam;
  using StatementType = ConnectionType::Statement;

  StatementType statement{
      this->connection_,
      std::format("SELECT * FROM {} WHERE a=? AND b=? AND c=?", table_name)};

  // All parameters are null.
  statement.BindNull(0);
  statement.BindNull(1);
  statement.BindNull(2);
  EXPECT_THAT(ReadAllRows(statement), ElementsAre());

  // Bind one parameter.
  statement.Bind(0, 10);
  statement.BindNull(1);
  statement.BindNull(2);
  EXPECT_THAT(ReadAllRows(statement), ElementsAre());

  // Bind all parameters.
  statement.Bind(0, 10);
  statement.Bind(1, 100);
  statement.Bind(2, "A");
  EXPECT_THAT(ReadAllRows(statement), ElementsAre(Row{10, 100, "A"}));
}

}  // namespace sql
