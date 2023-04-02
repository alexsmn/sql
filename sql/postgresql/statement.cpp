#include "sql/postgresql/statement.h"

#include "sql/exception.h"
#include "sql/postgresql/connection.h"
#include "sql/postgresql/conversions.h"
#include "sql/postgresql/field_view.h"
#include "sql/postgresql/postgres_util.h"
#include "sql/postgresql/result.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/locale/encoding_utf.hpp>
#include <cassert>
#include <ranges>

namespace sql::postgresql {

namespace {

const size_t AVG_PARAM_COUNT = 16;

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

statement::statement(connection& connection, std::string_view sql) {
  prepare(connection, sql);
}

statement::~statement() {
  close();
}

void statement::prepare(connection& connection, std::string_view sql) {
  assert(connection.conn_);

  auto name = connection.GenerateStatementName();

  std::string sanitized_sql{sql};
  ReplacePostgresParameters(sanitized_sql);

  {
    result res{PQprepare(connection.conn_, name.c_str(), sanitized_sql.c_str(),
                         0, nullptr)};
    CheckPostgresResult(res.get());
  }

  {
    result res{PQdescribePrepared(connection.conn_, name.c_str())};
    CheckPostgresResult(res.get());

    params_.resize(res.param_count());
    for (int i = 0; i < res.param_count(); ++i) {
      params_[i].type = res.param_type(i);
    }
  }

  connection_ = &connection;
  conn_ = connection.conn_;
  name_ = std::move(name);

#ifndef NDEBUG
  sql_ = std::move(sanitized_sql);
#endif
}

void statement::bind_null(unsigned column) {
  params_[column].buffer.clear();
}

void statement::bind(unsigned column, bool value) {
  SetBufferValue(static_cast<int64_t>(value ? 1 : 0), params_[column].type,
                 params_[column].buffer);
}

void statement::bind(unsigned column, int value) {
  SetBufferValue(static_cast<int64_t>(value), params_[column].type,
                 params_[column].buffer);
}

void statement::bind(unsigned column, int64_t value) {
  SetBufferValue(value, params_[column].type, params_[column].buffer);
}

void statement::bind(unsigned column, double value) {
  SetBufferValue(value, params_[column].type, params_[column].buffer);
}

void statement::bind(unsigned column, const char* value) {
  bind(column, std::string_view{value});
}

void statement::bind(unsigned column, const char16_t* value) {
  bind(column, std::u16string_view{value});
}

void statement::bind(unsigned column, std::string_view value) {
  SetBufferValue(value, params_[column].type, params_[column].buffer);
}

void statement::bind(unsigned column, std::u16string_view value) {
  SetBufferValue(boost::locale::conv::utf_to_utf<char>(
                     value.data(), value.data() + value.size()),
                 params_[column].type, params_[column].buffer);
}

size_t statement::field_count() const {
  assert(conn_);
  assert(false);
  return 0;
}

field_view statement::at(unsigned column) const {
  return field_view{result_, static_cast<int>(column)};
}

field_type statement::field_type(unsigned column) const {
  return at(column).type();
}

bool statement::get_bool(unsigned column) const {
  return at(column).get_bool();
}

int statement::get_int(unsigned column) const {
  return at(column).get_int();
}

int64_t statement::get_int64(unsigned column) const {
  return at(column).get_int64();
}

double statement::get_double(unsigned column) const {
  return at(column).get_double();
}

std::string statement::get_string(unsigned column) const {
  return at(column).get_string();
}

std::u16string statement::get_string16(unsigned column) const {
  return at(column).get_string16();
}

void statement::query() {
  assert(conn_);

  query(false);
}

bool statement::next() {
  assert(conn_);

  result_.reset();

  query(true);

  result_.reset(PQgetResult(conn_));
  if (!result_) {
    return false;
  }

  CheckPostgresResult(result_.get());

  return result_.status() == PGRES_SINGLE_TUPLE;
}

void statement::reset() {
  assert(conn_);

  result_.reset();
  std::ranges::for_each(params_, [](auto& param) { param.buffer.clear(); });

  for (;;) {
    result result{PQgetResult(conn_)};
    if (!result) {
      break;
    }
  }

  executed_ = false;
}

void statement::close() {
  result_.reset();

  if (!name_.empty()) {
    PQexec(conn_, std::format("DEALLOCATE {}", name_).c_str());
    name_ = {};
  }
}

void statement::query(bool single_row) {
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
