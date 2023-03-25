#pragma once

#include "sql/types.h"

#include <filesystem>
#include <vector>

namespace sql {

class Statement;

class Connection {
 public:
  Connection() = default;

  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  Connection(Connection&& source) noexcept : model_{std::move(source.model_)} {}
  Connection& operator=(Connection&& source) noexcept {
    model_ = std::move(source.model_);
    return *this;
  }

  void Open(const OpenParams& params);
  void Close() { model_->Close(); }

  void Execute(std::string_view sql) { model_->Execute(sql); }

  void BeginTransaction() { model_->BeginTransaction(); }
  void CommitTransaction() { model_->CommitTransaction(); }
  void RollbackTransaction() { model_->RollbackTransaction(); }

  int GetLastChangeCount() const { return model_->GetLastChangeCount(); }

  bool DoesTableExist(std::string_view table_name) const {
    return model_->DoesTableExist(table_name);
  }
  bool DoesColumnExist(std::string_view table_name,
                       std::string_view column_name) const {
    return model_->DoesColumnExist(table_name, column_name);
  }
  bool DoesIndexExist(std::string_view table_name,
                      std::string_view index_name) const {
    return model_->DoesIndexExist(table_name, index_name);
  }

  std::vector<Column> GetTableColumns(std::string_view table_name) const {
    return model_->GetTableColumns(std::string{table_name}.c_str());
  }

 private:
  class StatementModel {
   public:
    virtual ~StatementModel() = default;

    virtual bool IsInitialized() const = 0;

    virtual void BindNull(unsigned column) = 0;
    virtual void Bind(unsigned column, bool value) = 0;
    virtual void Bind(unsigned column, int value) = 0;
    virtual void Bind(unsigned column, int64_t value) = 0;
    virtual void Bind(unsigned column, double value) = 0;
    virtual void Bind(unsigned column, std::string_view value) = 0;
    virtual void Bind(unsigned column, std::u16string_view value) = 0;

    virtual size_t GetColumnCount() const = 0;
    virtual ColumnType GetColumnType(unsigned column) const = 0;

    virtual bool GetColumnBool(unsigned column) const = 0;
    virtual int GetColumnInt(unsigned column) const = 0;
    virtual int64_t GetColumnInt64(unsigned column) const = 0;
    virtual double GetColumnDouble(unsigned column) const = 0;
    virtual std::string GetColumnString(unsigned column) const = 0;
    virtual std::u16string GetColumnString16(unsigned column) const = 0;

    virtual void Run() = 0;
    virtual bool Step() = 0;
    virtual void Reset() = 0;

    virtual void Close() = 0;
  };

  class ConnectionModel {
   public:
    virtual ~ConnectionModel() = default;

    virtual void Open(const OpenParams& params) = 0;
    virtual void Close() = 0;

    virtual void Execute(std::string_view sql) = 0;

    virtual void BeginTransaction() = 0;
    virtual void CommitTransaction() = 0;
    virtual void RollbackTransaction() = 0;

    virtual int GetLastChangeCount() const = 0;

    virtual bool DoesTableExist(std::string_view table_name) const = 0;
    virtual bool DoesColumnExist(std::string_view table_name,
                                 std::string_view column_name) const = 0;
    virtual bool DoesIndexExist(std::string_view table_name,
                                std::string_view index_name) const = 0;

    virtual std::vector<Column> GetTableColumns(
        std::string_view table_name) const = 0;

    virtual std::unique_ptr<StatementModel> CreateStatementModel(
        std::string_view sql) = 0;
  };

  std::unique_ptr<ConnectionModel> model_;

  template <class ConnectionType, class StatementType>
  class ConnectionModelImpl;

  template <class ConnectionType, class StatementType>
  class StatementModelImpl;

  friend class Statement;
};

}  // namespace sql
