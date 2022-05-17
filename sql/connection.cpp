#include "sql/connection.h"

#include "sql/sqlite3/connection.h"
#include "sql/sqlite3/statement.h"

namespace sql {

template <class ConnectionType, class StatementType>
class Connection::ConnectionModelImpl : public ConnectionModel {
 public:
  virtual void set_exclusive_locking() override {
    connection_.set_exclusive_locking();
  }
  virtual void set_journal_size_limit(int limit) override {
    connection_.set_journal_size_limit(limit);
  }

  virtual void Open(const std::filesystem::path& path) override {
    connection_.Open(path);
  }
  virtual void Close() override { connection_.Close(); }

  virtual void Execute(const char* sql) override { connection_.Execute(sql); }

  virtual void BeginTransaction() override { connection_.BeginTransaction(); }
  virtual void CommitTransaction() override { connection_.CommitTransaction(); }
  virtual void RollbackTransaction() override {
    connection_.RollbackTransaction();
  }

  virtual int GetLastChangeCount() const override {
    return connection_.GetLastChangeCount();
  }

  virtual bool DoesTableExist(const char* table_name) const override {
    return connection_.DoesTableExist(table_name);
  }
  virtual bool DoesColumnExist(const char* table_name,
                               const char* column_name) const override {
    return connection_.DoesColumnExist(table_name, column_name);
  }
  virtual bool DoesIndexExist(const char* table_name,
                              const char* index_name) const override {
    return connection_.DoesIndexExist(table_name, index_name);
  }

  virtual std::unique_ptr<StatementModel> CreateStatementModel(
      const char* sql) override {
    StatementType statement;
    statement.Init(connection_, sql);
    return std::make_unique<StatementModelImpl<sqlite3::Statement>>(
        std::move(statement));
  }

 private:
  ConnectionType connection_;
};

template <class ConnectionType>
class Connection::StatementModelImpl : public StatementModel {
 public:
  explicit StatementModelImpl(ConnectionType&& statement)
      : statement_{std::move(statement)} {}

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
  virtual void Bind(unsigned column, const char* value) override {
    statement_.Bind(column, value);
  }
  virtual void Bind(unsigned column, const std::string& value) override {
    statement_.Bind(column, value);
  }
  virtual void Bind(unsigned column, const std::u16string& value) override {
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
  ConnectionType statement_;
};

Connection::Connection()
    : model_{std::make_unique<
          ConnectionModelImpl<sqlite3::Connection, sqlite3::Statement>>()} {}

}  // namespace sql
