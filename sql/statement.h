#pragma once

#include "sql/connection.h"
#include "sql/sqlite3/statement.h"

#include <memory>
#include <string>

namespace sql {

class Connection;

class Statement {
 public:
  Statement() = default;

  Statement(const Statement&) = delete;
  Statement& operator=(const Statement&) = delete;

  Statement(Statement&& source) noexcept : model_{std::move(source.model_)} {}
  Statement& operator=(Statement&& source) noexcept {
    model_ = std::move(source.model_);
    return *this;
  }

  bool IsInitialized() const { return model_ && model_->IsInitialized(); };

  void Init(Connection& connection, const char* sql) {
    model_ = connection.model_->CreateStatementModel(sql);
  }

  void BindNull(unsigned column) { model_->BindNull(column); }
  void Bind(unsigned column, bool value) { model_->Bind(column, value); }
  void Bind(unsigned column, int value) { model_->Bind(column, value); }
  void Bind(unsigned column, int64_t value) { model_->Bind(column, value); }
  void Bind(unsigned column, double value) { model_->Bind(column, value); }
  void Bind(unsigned column, const char* value) { model_->Bind(column, value); }
  void Bind(unsigned column, const std::string& value) {
    model_->Bind(column, value);
  }
  void Bind(unsigned column, const std::u16string& value) {
    model_->Bind(column, value);
  }

  size_t GetColumnCount() const { return model_->GetColumnCount(); }
  ColumnType GetColumnType(unsigned column) const {
    return model_->GetColumnType(column);
  }

  bool GetColumnBool(unsigned column) const {
    return model_->GetColumnBool(column);
  }
  int GetColumnInt(unsigned column) const {
    return model_->GetColumnInt(column);
  }
  int64_t GetColumnInt64(unsigned column) const {
    return model_->GetColumnInt64(column);
  }
  double GetColumnDouble(unsigned column) const {
    return model_->GetColumnDouble(column);
  }
  std::string GetColumnString(unsigned column) const {
    return model_->GetColumnString(column);
  }
  std::u16string GetColumnString16(unsigned column) const {
    return model_->GetColumnString16(column);
  }

  void Run() { model_->Run(); }
  bool Step() { return model_->Step(); }
  void Reset() { model_->Reset(); }

  void Close() { model_->Close(); }

 private:
  std::unique_ptr<Connection::StatementModel> model_;
};

}  // namespace sql
