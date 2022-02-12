#pragma once

#include <filesystem>

#include "sql/types.h"

namespace sql {

class Statement;

class Connection {
 public:
  Connection();

  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  void set_exclusive_locking() { model_->set_exclusive_locking(); }
  void set_journal_size_limit(int limit) {
    model_->set_journal_size_limit(limit);
  }

  void Open(const std::filesystem::path& path) { model_->Open(path); }
  void Close() { model_->Close(); }

  void Execute(const char* sql) { model_->Execute(sql); }

  void BeginTransaction() { model_->BeginTransaction(); }
  void CommitTransaction() { model_->CommitTransaction(); }
  void RollbackTransaction() { model_->RollbackTransaction(); }

  int GetLastChangeCount() const { return model_->GetLastChangeCount(); }

  bool DoesTableExist(const char* table_name) const {
    return model_->DoesTableExist(table_name);
  }
  bool DoesColumnExist(const char* table_name, const char* column_name) const {
    return model_->DoesColumnExist(table_name, column_name);
  }
  bool DoesIndexExist(const char* table_name, const char* index_name) const {
    return model_->DoesIndexExist(table_name, index_name);
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
    virtual void Bind(unsigned column, const char* value) = 0;
    virtual void Bind(unsigned column, const std::string& value) = 0;
    virtual void Bind(unsigned column, const std::wstring& value) = 0;

    virtual size_t GetColumnCount() const = 0;
    virtual ColumnType GetColumnType(unsigned column) const = 0;

    virtual bool GetColumnBool(unsigned column) const = 0;
    virtual int GetColumnInt(unsigned column) const = 0;
    virtual int64_t GetColumnInt64(unsigned column) const = 0;
    virtual double GetColumnDouble(unsigned column) const = 0;
    virtual std::string GetColumnString(unsigned column) const = 0;
    virtual std::wstring GetColumnString16(unsigned column) const = 0;

    virtual void Run() = 0;
    virtual bool Step() = 0;
    virtual void Reset() = 0;

    virtual void Close() = 0;
  };

  class ConnectionModel {
   public:
    virtual ~ConnectionModel() = default;

    virtual void set_exclusive_locking() = 0;
    virtual void set_journal_size_limit(int limit) = 0;

    virtual void Open(const std::filesystem::path& path) = 0;
    virtual void Close() = 0;

    virtual void Execute(const char* sql) = 0;

    virtual void BeginTransaction() = 0;
    virtual void CommitTransaction() = 0;
    virtual void RollbackTransaction() = 0;

    virtual int GetLastChangeCount() const = 0;

    virtual bool DoesTableExist(const char* table_name) const = 0;
    virtual bool DoesColumnExist(const char* table_name,
                                 const char* column_name) const = 0;
    virtual bool DoesIndexExist(const char* table_name,
                                const char* index_name) const = 0;

    virtual std::unique_ptr<StatementModel> CreateStatementModel(
        const char* sql) = 0;
  };

  std::unique_ptr<ConnectionModel> model_;

  template <class T, class StatementType>
  class ConnectionModelImpl;

  template <class T>
  class StatementModelImpl;

  friend class Statement;
};

}  // namespace sql
