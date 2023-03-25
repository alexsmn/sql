#include "sql/postgresql/statement.h"

#include "sql/exception.h"
#include "sql/postgresql/connection.h"
#include "sql/postgresql/postgres_util.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/locale/encoding_utf.hpp>
#include <cassert>
#include <ranges>
#include <sqlite3.h>

namespace sql::postgresql {

namespace {

const size_t AVG_PARAM_COUNT = 16;

template <class T>
T GetResultValue(PGresult* result, unsigned column) {
  assert(result);
  assert(PQfformat(result, column) == 1);
  assert(PQfsize(result, column) == sizeof(T));
  auto* buffer = PQgetvalue(result, 0, column);
  assert(buffer);
  T value;
  memcpy(&value, buffer, sizeof(T));
  return value;
}

template <class T>
void SetBuffer(std::vector<char>& buffer, const T& value) {
  buffer.resize(sizeof(T));
  memcpy(buffer.data(), &value, sizeof(T));
}

void ReplacePostgresParameters(std::string& sql) {
  size_t pos = 0;
  for (int index = 1;;++index) {
    auto q = sql.find('?', pos);
    if (q == sql.npos) {
      break;
    }
    auto param = std::format("${}", index);
    sql.replace(q, 1, param);
    pos = q + param.size();
  }
}

}  // namespace

Statement::~Statement() {
  Close();
}

void Statement::Init(Connection& connection, std::string_view sql) {
  assert(connection.conn_);

  connection_ = &connection;
  conn_ = connection.conn_;
  name_ = connection.GenerateStatementName();
  sql_ = sql;

  ReplacePostgresParameters(sql_);
}

void Statement::BindNull(unsigned column) {
  GetParam(column);
}

void Statement::Bind(unsigned column, bool value) {
  Bind(column, value ? 1 : 0);
}

void Statement::Bind(unsigned column, int value) {
  assert(conn_);
  SetBuffer(GetParam(column).value, boost::endian::native_to_big(value));
}

void Statement::Bind(unsigned column, int64_t value) {
  assert(conn_);
  SetBuffer(GetParam(column).value, boost::endian::native_to_big(value));
}

void Statement::Bind(unsigned column, double value) {
  assert(conn_);
  SetBuffer(GetParam(column).value, value);
}

void Statement::Bind(unsigned column, std::string_view value) {
  GetParam(column).value.assign(value.begin(), value.end());
}

void Statement::Bind(unsigned column, std::u16string_view value) {
  Bind(column, boost::locale::conv::utf_to_utf<char>(
                   value.data(), value.data() + value.size()));
}

size_t Statement::GetColumnCount() const {
  assert(conn_);
  assert(false);
  return 0;
}

ColumnType Statement::GetColumnType(unsigned column) const {
  // TODO: Implement.
  assert(false);
  return COLUMN_TYPE_NULL;
}

bool Statement::GetColumnBool(unsigned column) const {
  return GetColumnInt(column) != 0;
}

int Statement::GetColumnInt(unsigned column) const {
  return boost::endian::big_to_native(GetResultValue<int32_t>(result_, column));
}

int64_t Statement::GetColumnInt64(unsigned column) const {
  return boost::endian::big_to_native(GetResultValue<int64_t>(result_, column));
}

double Statement::GetColumnDouble(unsigned column) const {
  return GetResultValue<double>(result_, column);
}

std::string Statement::GetColumnString(unsigned column) const {
  assert(result_);
  assert(PQfformat(result_, column) == 1);
  auto* buffer = PQgetvalue(result_, 0, column);
  return buffer ? std::string{buffer} : std::string{};
}

std::u16string Statement::GetColumnString16(unsigned column) const {
  std::string string = GetColumnString(column);
  return boost::locale::conv::utf_to_utf<char16_t>(string);
}

void Statement::Run() {
  assert(conn_);

  Prepare();
  Execute(false);
}

bool Statement::Step() {
  assert(conn_);

  if (result_) {
    PQclear(result_);
    result_ = nullptr;
  }

  Prepare();
  Execute(true);

  result_ = PQgetResult(conn_);
  if (!result_) {
    return false;
  }

  CheckPostgresResult(result_);

  return PQresultStatus(result_) == PGRES_SINGLE_TUPLE;
}

void Statement::Reset() {
  assert(conn_);

  if (result_) {
    PQclear(result_);
    result_ = nullptr;
  }

  for (;;) {
    result_ = PQgetResult(conn_);
    if (!result_) {
      break;
    }
    PQclear(result_);
    result_ = nullptr;
  }

  executed_ = false;
}

void Statement::Close() {
  if (result_) {
    PQclear(result_);
    result_ = nullptr;
  }

  if (!name_.empty()) {
    PQexec(conn_, std::format("DEALLOCATE {}", name_).c_str());
    name_ = {};
  }
}

Statement::Param& Statement::GetParam(unsigned column) {
  if (params_.size() <= column) {
    params_.resize(column + 1);
  }
  return params_[column];
}

void Statement::Prepare() {
  assert(conn_);

  if (prepared_) {
    return;
  }

  CheckPostgresResult(PQprepare(conn_, name_.c_str(), sql_.c_str(),
                                static_cast<int>(params_.size()), nullptr));

  prepared_ = true;
}

void Statement::Execute(bool single_row) {
  if (executed_) {
    return;
  }

  boost::container::static_vector<const char*, AVG_PARAM_COUNT> param_values(
      params_.size());
  std::ranges::transform(params_, param_values.begin(),
                         [](auto&& p) { return p.value.data(); });

  boost::container::static_vector<int, AVG_PARAM_COUNT> param_lengths(
      params_.size());
  std::ranges::transform(params_, param_lengths.begin(), [](auto&& p) {
    return static_cast<int>(p.value.size());
  });

  boost::container::static_vector<int, AVG_PARAM_COUNT> param_formats(
      params_.size(), 1);

  if (single_row) {
    int result = PQsendQueryPrepared(
        conn_, name_.c_str(), static_cast<int>(param_values.size()),
        param_values.data(), param_lengths.data(), param_formats.data(), 1);

    if (result != 1) {
      throw Exception{"Cannot send prepared query"};
    }

    if (PQsetSingleRowMode(conn_) != 1) {
      throw Exception{"Cannot enter single row mode"};
    }

  } else {
    result_ = PQexecPrepared(
        conn_, name_.c_str(), static_cast<int>(param_values.size()),
        param_values.data(), param_lengths.data(), param_formats.data(), 1);

    CheckPostgresResult(result_);
  }

  executed_ = true;
}

}  // namespace sql::postgresql
