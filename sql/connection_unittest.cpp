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
struct connection_traits;

template <>
struct connection_traits<sql::sqlite3::connection> {
  sql::open_params GetOpenParams() {
    return {.driver = "sqlite", .path = temp_dir_.get() / "database.sqlite3"};
  }

  ScopedTempDir temp_dir_;
};

template <>
struct connection_traits<sql::postgresql::connection> {
  sql::open_params GetOpenParams() {
    return {
        .driver = "postgres",
        .connection_string =
            "host=localhost port=5433 dbname=test user=postgres password=1234"};
  }
};

template <>
struct connection_traits<sql::connection>
    : connection_traits<sql::sqlite3::connection> {};

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
  using StatementType = typename T::statement;

  virtual void SetUp() override;
  virtual void TearDown() override;

 protected:
  void InsertTestData(std::span<const Row> rows);

  T connection_;
  connection_traits<T> connection_traits_;

  std::string table_name_;
};

using connection_types = ::testing::Types<sql::connection,
                                          sql::sqlite3::connection,
                                          sql::postgresql::connection>;
TYPED_TEST_SUITE(ConnectionTest, connection_types);

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
    if (!connection.table_exists(table_name)) {
      return table_name;
    }
  }
  throw std::runtime_error{"Cannot create a temp table"};
}

template <class T>
void ConnectionTest<T>::SetUp() {
  this->connection_.open(this->connection_traits_.GetOpenParams());

  table_name_ = GetTempTableName(this->connection_);

  this->connection_.query(
      std::format("CREATE TABLE {}(A INTEGER, B BIGINT, C TEXT)", table_name_));
}

template <class T>
void ConnectionTest<T>::TearDown() {
  if (this->connection_.table_exists(table_name_)) {
    this->connection_.query(std::format("DROP TABLE {}", table_name_));
  }

  connection_.close();
}

TYPED_TEST(ConnectionTest, TestColumns) {
  const auto& table_name = this->table_name_;

  EXPECT_TRUE(this->connection_.table_exists(table_name));
  EXPECT_TRUE(this->connection_.field_exists(table_name, "A"));
  EXPECT_TRUE(this->connection_.field_exists(table_name, "B"));
  EXPECT_TRUE(this->connection_.field_exists(table_name, "C"));
  EXPECT_FALSE(this->connection_.field_exists(table_name, "D"));

  EXPECT_THAT(
      this->connection_.table_fields(table_name),
      UnorderedElementsAre(FieldsAre(StrCaseEq("A"), field_type::INTEGER),
                           FieldsAre(StrCaseEq("B"), field_type::INTEGER),
                           FieldsAre(StrCaseEq("C"), field_type::TEXT)));
}

TYPED_TEST(ConnectionTest, TestIndexes) {
  const auto& table_name = this->table_name_;
  const auto index_name = std::format("{}_index", table_name);
  const auto missing_index_name = std::format("{}_missing_index", table_name);

  EXPECT_FALSE(this->connection_.index_exists(table_name, index_name));

  this->connection_.query(
      std::format("CREATE INDEX {} ON {}(A)", index_name, table_name));

  EXPECT_TRUE(this->connection_.index_exists(table_name, index_name));
  EXPECT_FALSE(this->connection_.index_exists(table_name, missing_index_name));
}

template <class T>
void ConnectionTest<T>::InsertTestData(std::span<const Row> rows) {
  StatementType insert_statement{
      connection_, std::format("INSERT INTO {} VALUES(?, ?, ?)", table_name_)};

  for (auto& row : rows) {
    insert_statement.bind(0, row.a);
    insert_statement.bind(1, row.b);
    insert_statement.bind(2, row.c);
    insert_statement.query();
    EXPECT_EQ(1, connection_.last_change_count());
    insert_statement.reset();
  }
}

template <class T>
Row ReadRow(T& statement) {
  std::vector<Row> rows;

  while (statement.next()) {
    EXPECT_EQ(field_type::INTEGER, statement.field_type(0));
    EXPECT_EQ(field_type::INTEGER, statement.field_type(1));
    EXPECT_EQ(field_type::TEXT, statement.field_type(2));
    auto a = statement.get_int(0);
    auto b = statement.get_int64(1);
    auto c = statement.get_string(2);
    rows.emplace_back(a, b, std::move(c));
  }

  statement.reset();

  return rows;
}

template <class T>
std::vector<Row> ReadAllRows(T& statement) {
  std::vector<Row> rows;

  while (statement.next()) {
    EXPECT_EQ(field_type::INTEGER, statement.field_type(0));
    EXPECT_EQ(field_type::INTEGER, statement.field_type(1));
    EXPECT_EQ(field_type::TEXT, statement.field_type(2));
    auto a = statement.at(0).as_int();
    auto b = statement.at(1).as_int64();
    auto c = statement.at(2).as_string();
    rows.emplace_back(a, b, std::move(c));
  }

  statement.reset();

  return rows;
}

TYPED_TEST(ConnectionTest, TestStatements) {
  const auto& table_name = this->table_name_;

  auto initial_rows = GenerateRows();
  this->InsertTestData(initial_rows);

  using ConnectionType = TypeParam;
  using StatementType = ConnectionType::statement;

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
  using StatementType = ConnectionType::statement;

  StatementType statement{
      this->connection_,
      std::format("SELECT * FROM {} WHERE a=? AND b=? AND c=?", table_name)};

  // All parameters are null.
  statement.bind_null(0);
  statement.bind_null(1);
  statement.bind_null(2);
  EXPECT_THAT(ReadAllRows(statement), ElementsAre());

  // bind one parameter.
  statement.bind(0, 10);
  statement.bind_null(1);
  statement.bind_null(2);
  EXPECT_THAT(ReadAllRows(statement), ElementsAre());

  // bind all parameters.
  statement.bind(0, 10);
  statement.bind(1, 100);
  statement.bind(2, "A");
  EXPECT_THAT(ReadAllRows(statement), ElementsAre(Row{10, 100, "A"}));
}

}  // namespace sql
