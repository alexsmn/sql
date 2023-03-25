#include "sql/connection.h"

#include "sql/postgresql/connection.h"
#include "sql/postgresql/statement.h"
#include "sql/sqlite3/connection.h"
#include "sql/sqlite3/statement.h"

#include <cassert>

namespace sql {

template <class ConnectionType, class StatementType>
class Connection::ConnectionModelImpl : public ConnectionModel {
 public:
  virtual void Open(const OpenParams& params) override {
    connection_.Open(params);
  }

  virtual void Close() override { connection_.Close(); }

  virtual void Execute(std::string_view sql) override {
    connection_.Execute(sql);
  }

  virtual void BeginTransaction() override { connection_.BeginTransaction(); }

  virtual void CommitTransaction() override { connection_.CommitTransaction(); }

  virtual void RollbackTransaction() override {
    connection_.RollbackTransaction();
  }

  virtual int GetLastChangeCount() const override {
    return connection_.GetLastChangeCount();
  }

  virtual bool DoesTableExist(std::string_view table_name) const override {
    return connection_.DoesTableExist(table_name);
  }

  virtual bool DoesColumnExist(std::string_view table_name,
                               std::string_view column_name) const override {
    return connection_.DoesColumnExist(table_name, column_name);
  }

  virtual bool DoesIndexExist(std::string_view table_name,
                              std::string_view index_name) const override {
    return connection_.DoesIndexExist(table_name, index_name);
  }

  virtual std::vector<Column> GetTableColumns(
      std::string_view table_name) const override {
    return connection_.GetTableColumns(table_name);
  }

  virtual std::unique_ptr<StatementModel> CreateStatementModel(
      std::string_view sql) override {
    return std::make_unique<StatementModelImpl<ConnectionType, StatementType>>(
        connection_, sql);
  }

 private:
  ConnectionType connection_;
};

template <class ConnectionType, class StatementType>
class Connection::StatementModelImpl : public StatementModel {
 public:
  StatementModelImpl() = default;

  StatementModelImpl(ConnectionType& connection, std::string_view sql)
      : statement_{connection, sql} {}

  virtual bool IsInitialized() const override {
    return statement_.IsInitialized();
  };

  virtual void BindNull(unsigned column) override {
    statement_.BindNull(column);
  }
  virtual void Bind(unsigned column, bool value) override {
    statement_.Bind(column, value);
  }
  virtual void Bind(unsigned column, int value) override {
    statement_.Bind(column, value);
  }
  virtual void Bind(unsigned column, int64_t value) override {
    statement_.Bind(column, value);
  }
  virtual void Bind(unsigned column, double value) override {
    statement_.Bind(column, value);
  }
  virtual void Bind(unsigned column, std::string_view value) override {
    statement_.Bind(column, value);
  }
  virtual void Bind(unsigned column, std::u16string_view value) override {
    statement_.Bind(column, value);
  }

  virtual size_t GetColumnCount() const override {
    return statement_.GetColumnCount();
  }

  virtual ColumnType GetColumnType(unsigned column) const override {
    return statement_.GetColumnType(column);
  }

  virtual bool GetColumnBool(unsigned column) const override {
    return statement_.GetColumnBool(column);
  }
  virtual int GetColumnInt(unsigned column) const override {
    return statement_.GetColumnInt(column);
  }
  virtual int64_t GetColumnInt64(unsigned column) const override {
    return statement_.GetColumnInt64(column);
  }
  virtual double GetColumnDouble(unsigned column) const override {
    return statement_.GetColumnDouble(column);
  }
  virtual std::string GetColumnString(unsigned column) const override {
    return statement_.GetColumnString(column);
  }
  virtual std::u16string GetColumnString16(unsigned column) const override {
    return statement_.GetColumnString16(column);
  }

  virtual void Run() override { statement_.Run(); }
  virtual bool Step() override { return statement_.Step(); }
  virtual void Reset() override { statement_.Reset(); }
  virtual void Close() override { statement_.Close(); }

 private:
  StatementType statement_;
};

void Connection::Open(const OpenParams& params) {
  assert(!model_);

  if (params.driver.empty() || params.driver == "sqlite" ||
      params.driver == "sqlite3") {
    model_ = std::make_unique<
        ConnectionModelImpl<sqlite3::Connection, sqlite3::Statement>>();
  } else if (params.driver == "postgres" || params.driver == "postgresql") {
    model_ = std::make_unique<
        ConnectionModelImpl<postgresql::Connection, postgresql::Statement>>();
  } else {
    throw std::runtime_error{"Unknown SQL driver"};
  }

  model_->Open(params);
}

}  // namespace sql
