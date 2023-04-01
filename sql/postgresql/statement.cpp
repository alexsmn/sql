#include "sql/postgresql/statement.h"

#include "sql/exception.h"
#include "sql/postgresql/connection.h"
#include "sql/postgresql/field_view.h"
#include "sql/postgresql/postgres_util.h"
#include "sql/postgresql/result.h"

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

void SetParamInt64(int64_t value,
                   Oid type,
                   boost::container::small_vector<char, 8>& buffer) {
  switch (type) {
    case BOOLOID:
      // TODO: Validate value narrowing.
      SetBuffer(buffer, boost::endian::native_to_big(
                            static_cast<int32_t>(value ? 1 : 0)));
      return;
    case INT4OID:
      // TODO: Validate value narrowing.
      SetBuffer(buffer,
                boost::endian::native_to_big(static_cast<int32_t>(value)));
      return;
    case INT8OID:
      SetBuffer(buffer, boost::endian::native_to_big(value));
      return;
    default:
      assert(false);
  }
}

void SetParamDouble(double value,
                    Oid type,
                    boost::container::small_vector<char, 8>& buffer) {
  switch (type) {
    case FLOAT4OID:
      // TODO: Validate value narrowing.
      SetBuffer(buffer, static_cast<float>(value));
      return;
    case FLOAT8OID:
      SetBuffer(buffer, value);
      return;
    default:
      assert(false);
  }
}

void SetParamString(std::string_view str,
                    Oid type,
                    boost::container::small_vector<char, 8>& buffer) {
  switch (type) {
    case NAMEOID:
    case TEXTOID:
      buffer.assign(str.begin(), str.end());
      return;
    default:
      assert(false);
  }
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

  auto name = connection.GenerateStatementName();

  std::string sanitized_sql{sql};
  ReplacePostgresParameters(sanitized_sql);

  Result result{PQprepare(connection.conn_, name.c_str(), sanitized_sql.c_str(),
                          0, nullptr)};
  CheckPostgresResult(result.get());

  {
    Result result{PQdescribePrepared(connection.conn_, name.c_str())};
    CheckPostgresResult(result.get());

    params_.resize(result.param_count());
    for (int i = 0; i < result.param_count(); ++i) {
      params_[i].type = result.param_type(i);
    }
  }

  connection_ = &connection;
  conn_ = connection.conn_;
  name_ = std::move(name);

#ifndef NDEBUG
  sql_ = std::move(sanitized_sql);
#endif
}

void Statement::BindNull(unsigned column) {
  params_[column].buffer.clear();
}

void Statement::Bind(unsigned column, bool value) {
  SetParamInt64(value ? 1 : 0, params_[column].type, params_[column].buffer);
}

void Statement::Bind(unsigned column, int value) {
  SetParamInt64(value, params_[column].type, params_[column].buffer);
}

void Statement::Bind(unsigned column, int64_t value) {
  SetParamInt64(value, params_[column].type, params_[column].buffer);
}

void Statement::Bind(unsigned column, double value) {
  SetParamDouble(value, params_[column].type, params_[column].buffer);
}

void Statement::Bind(unsigned column, const char* value) {
  Bind(column, std::string_view{value});
}

void Statement::Bind(unsigned column, const char16_t* value) {
  Bind(column, std::u16string_view{value});
}

void Statement::Bind(unsigned column, std::string_view value) {
  SetParamString(value, params_[column].type, params_[column].buffer);
}

void Statement::Bind(unsigned column, std::u16string_view value) {
  SetParamString(boost::locale::conv::utf_to_utf<char>(
                     value.data(), value.data() + value.size()),
                 params_[column].type, params_[column].buffer);
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

  Execute(false);
}

bool Statement::Step() {
  assert(conn_);

  result_.reset();

  Execute(true);

  result_.reset(PQgetResult(conn_));
  if (!result_) {
    return false;
  }

  CheckPostgresResult(result_.get());

  return result_.status() == PGRES_SINGLE_TUPLE;
}

void Statement::Reset() {
  assert(conn_);

  result_.reset();
  std::ranges::for_each(params_, [](auto& param) { param.buffer.clear(); });

  for (;;) {
    Result result{PQgetResult(conn_)};
    if (!result) {
      break;
    }
  }

  executed_ = false;
}

void Statement::Close() {
  result_.reset();

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
      const char* error_message = PQerrorMessage(conn_);
      throw Exception{error_message};
    }

    if (PQsetSingleRowMode(conn_) != PGRES_COMMAND_OK) {
      const char* error_message = PQerrorMessage(conn_);
      throw Exception{error_message};
    }

  } else {
    result_.reset(PQexecPrepared(
        conn_, name_.c_str(), static_cast<int>(param_values.size()),
        param_values.data(), param_lengths.data(), param_formats.data(), 1));

    CheckPostgresResult(result_.get());

    assert(connection_);
    connection_->last_change_count_ = result_.affected_row_count();
  }

  executed_ = true;
}

}  // namespace sql::postgresql
