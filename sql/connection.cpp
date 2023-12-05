#include "sql/connection.h"

#include "sql/postgresql/connection.h"
#include "sql/postgresql/statement.h"
#include "sql/sqlite3/connection.h"
#include "sql/sqlite3/statement.h"

#include <cassert>

namespace sql {

template <class ConnectionType, class StatementType>
class connection::connection_model_impl : public connection_model {
 public:
  virtual void open(const open_params& params) override {
    connection_.open(params);
  }

  virtual void close() override { connection_.close(); }

  virtual void query(std::string_view sql) override { connection_.query(sql); }

  virtual void start() override { connection_.start(); }

  virtual void commit() override { connection_.commit(); }

  virtual void rollback() override { connection_.rollback(); }

  virtual int last_change_count() const override {
    return connection_.last_change_count();
  }

  virtual bool table_exists(std::string_view table_name) const override {
    return connection_.table_exists(table_name);
  }

  virtual bool field_exists(std::string_view table_name,
                            std::string_view column_name) const override {
    return connection_.field_exists(table_name, column_name);
  }

  virtual bool index_exists(std::string_view table_name,
                            std::string_view index_name) const override {
    return connection_.index_exists(table_name, index_name);
  }

  virtual std::vector<field_info> table_fields(
      std::string_view table_name) const override {
    return connection_.table_fields(table_name);
  }

  virtual std::unique_ptr<statement_model> create_statement_model(
      std::string_view sql) override {
    return std::make_unique<
        statement_model_impl<ConnectionType, StatementType>>(connection_, sql);
  }

 private:
  ConnectionType connection_;
};

template <class ConnectionType, class StatementType>
class connection::statement_model_impl : public statement_model {
 public:
  statement_model_impl() = default;

  statement_model_impl(ConnectionType& connection, std::string_view sql)
      : statement_{connection, sql} {}

  virtual bool is_prepared() const override {
    return statement_.is_prepared();
  };

  virtual void bind_null(unsigned column) override {
    statement_.bind_null(column);
  }

  virtual void bind(unsigned column, bool value) override {
    statement_.bind(column, value);
  }

  virtual void bind(unsigned column, int value) override {
    statement_.bind(column, value);
  }

  virtual void bind(unsigned column, int64_t value) override {
    statement_.bind(column, value);
  }

  virtual void bind(unsigned column, double value) override {
    statement_.bind(column, value);
  }

  virtual void bind(unsigned column, const char* value) override {
    statement_.bind(column, value);
  }

  virtual void bind(unsigned column, const char16_t* value) override {
    statement_.bind(column, value);
  }

  virtual void bind(unsigned column, std::string_view value) override {
    statement_.bind(column, value);
  }

  virtual void bind(unsigned column, std::u16string_view value) override {
    statement_.bind(column, value);
  }

  virtual size_t field_count() const override {
    return statement_.field_count();
  }

  virtual sql::field_type type(unsigned column) const override {
    return statement_.type(column);
  }

  virtual bool as_bool(unsigned column) const override {
    return statement_.at(column).as_bool();
  }

  virtual int as_int(unsigned column) const override {
    return statement_.at(column).as_int();
  }

  virtual int64_t as_int64(unsigned column) const override {
    return statement_.at(column).as_int64();
  }

  virtual double as_double(unsigned column) const override {
    return statement_.at(column).as_double();
  }

  virtual std::string_view as_string_view(unsigned column) const override {
    return statement_.at(column).as_string_view();
  }

  virtual std::string as_string(unsigned column) const override {
    return statement_.at(column).as_string();
  }

  virtual std::u16string as_string16(unsigned column) const override {
    return statement_.at(column).as_string16();
  }

  virtual void query() override { statement_.query(); }
  virtual bool next() override { return statement_.next(); }
  virtual void reset() override { statement_.reset(); }
  virtual void close() override { statement_.close(); }

 private:
  StatementType statement_;
};

connection::connection(const open_params& params) {
  open(params);
}

void connection::open(const open_params& params) {
  assert(!model_);

  if (params.driver.empty() || params.driver == "sqlite" ||
      params.driver == "sqlite3") {
    model_ = std::make_unique<
        connection_model_impl<sqlite3::connection, sqlite3::statement>>();
  } else if (params.driver == "postgres" || params.driver == "postgresql") {
    model_ = std::make_unique<
        connection_model_impl<postgresql::connection, postgresql::statement>>();
  } else {
    throw std::runtime_error{"Unknown SQL driver"};
  }

  model_->open(params);
}

}  // namespace sql
