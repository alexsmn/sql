#include "sql/postgresql/statement.h"

#include "sql/exception.h"
#include "sql/postgresql/connection.h"
#include "sql/postgresql/field_view.h"
#include "sql/postgresql/postgres_util.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/locale/encoding_utf.hpp>
#include <cassert>
#include <catalog/pg_type_d.h>
#include <ranges>

namespace sql::postgresql {

namespace {

const size_t AVG_PARAM_COUNT = 16;

template <class T>
void SetBuffer(boost::container::small_vector<char, 8>& buffer,
               const T& value) {
  buffer.resize(sizeof(T));
  memcpy(buffer.data(), &value, sizeof(T));
}

// Returns parameter count.
// TODO: Optimize.
size_t ReplacePostgresParameters(std::string& sql) {
  size_t pos = 0;
  for (size_t index = 1;; ++index) {
    auto q = sql.find('?', pos);
    if (q == sql.npos) {
      return index - 1;
    }
    auto param = std::format("${}", index);
    sql.replace(q, 1, param);
    pos = q + param.size();
  }
  return 0;
}

}  // namespace

Statement::Statement(Connection& connection, std::string_view sql) {
  Init(connection, sql);
}

Statement::~Statement() {
  Close();
}

void Statement::Init(Connection& connection, std::string_view sql) {
  assert(connection.conn_);

  connection_ = &connection;
  conn_ = connection.conn_;
  name_ = connection.GenerateStatementName();
  sql_ = sql;

  auto param_count = ReplacePostgresParameters(sql_);
  params_.resize(param_count);
}

void Statement::BindNull(unsigned column) {
  GetParamBuffer(column, InvalidOid);
}

void Statement::Bind(unsigned column, bool value) {
  SetBuffer(GetParamBuffer(column, BOOLOID),
            boost::endian::native_to_big(value ? 1 : 0));
}

void Statement::Bind(unsigned column, int value) {
  SetBuffer(GetParamBuffer(column, INT4OID),
            boost::endian::native_to_big(value));
}

void Statement::Bind(unsigned column, int64_t value) {
  SetBuffer(GetParamBuffer(column, INT8OID),
            boost::endian::native_to_big(value));
}

void Statement::Bind(unsigned column, double value) {
  SetBuffer(GetParamBuffer(column, FLOAT8OID), value);
}

void Statement::Bind(unsigned column, std::string_view value) {
  GetParamBuffer(column, TEXTOID).assign(value.begin(), value.end());
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

FieldView Statement::Get(unsigned column) const {
  return FieldView{result_, static_cast<int>(column)};
}

ColumnType Statement::GetColumnType(unsigned column) const {
  return Get(column).GetType();
}

bool Statement::GetColumnBool(unsigned column) const {
  return Get(column).GetBool();
}

int Statement::GetColumnInt(unsigned column) const {
  return Get(column).GetInt();
}

int64_t Statement::GetColumnInt64(unsigned column) const {
  return Get(column).GetInt64();
}

double Statement::GetColumnDouble(unsigned column) const {
  return Get(column).GetDouble();
}

std::string Statement::GetColumnString(unsigned column) const {
  return Get(column).GetString();
}

std::u16string Statement::GetColumnString16(unsigned column) const {
  return Get(column).GetString16();
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

Statement::ParamBuffer& Statement::GetParamBuffer(unsigned column, Oid type) {
  if (params_.size() <= column) {
    throw Exception{
        "The parameter index exceeds the parameter count in the prepared SQL "
        "statement"};
  }
  auto& param = params_[column];
  param.type = type;
  param.buffer.clear();
  return param.buffer;
}

void Statement::Prepare() {
  assert(conn_);

  if (prepared_) {
    return;
  }

  boost::container::small_vector<Oid, AVG_PARAM_COUNT> param_types(
      params_.size());
  std::ranges::transform(params_, param_types.begin(),
                         [](auto&& p) { return p.type; });

  CheckPostgresResult(PQprepare(conn_, name_.c_str(), sql_.c_str(),
                                static_cast<int>(params_.size()),
                                param_types.data()));

  prepared_ = true;
}

void Statement::Execute(bool single_row) {
  if (executed_) {
    return;
  }

  boost::container::small_vector<const char*, AVG_PARAM_COUNT> param_values(
      params_.size());
  std::ranges::transform(params_, param_values.begin(), [](auto&& p) {
    // |small_vector| returns non-null data even when is empty.
    return p.buffer.empty() ? nullptr : p.buffer.data();
  });

  boost::container::small_vector<int, AVG_PARAM_COUNT> param_lengths(
      params_.size());
  std::ranges::transform(params_, param_lengths.begin(), [](auto&& p) {
    return static_cast<int>(p.buffer.size());
  });

  boost::container::small_vector<int, AVG_PARAM_COUNT> param_formats(
      params_.size(), 1);

  if (single_row) {
    int result = PQsendQueryPrepared(
        conn_, name_.c_str(), static_cast<int>(param_values.size()),
        param_values.data(), param_lengths.data(), param_formats.data(), 1);

    if (result != PGRES_COMMAND_OK) {
      throw Exception{"Cannot send prepared query"};
    }

    if (PQsetSingleRowMode(conn_) != PGRES_COMMAND_OK) {
      throw Exception{"Cannot enter single row mode"};
    }

  } else {
    result_ = PQexecPrepared(
        conn_, name_.c_str(), static_cast<int>(param_values.size()),
        param_values.data(), param_lengths.data(), param_formats.data(), 1);

    CheckPostgresResult(result_);
  }

  assert(connection_);
  connection_->last_change_count_ = atoi(PQcmdTuples(result_));

  executed_ = true;
}

}  // namespace sql::postgresql
