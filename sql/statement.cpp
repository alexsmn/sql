#include "sql/statement.h"

#include "sql/exception.h"

namespace sql {

Statement::Statement(Connection& connection, std::string_view sql) {
  Init(connection, sql);
}

bool Statement::IsInitialized() const {
  return model_ && model_->IsInitialized();
};

void Statement::Init(Connection& connection, std::string_view sql) {
  model_ = connection.model_->CreateStatementModel(sql);
}

void Statement::BindNull(unsigned column) {
  model_->BindNull(column);
}

void Statement::Bind(unsigned column, bool value) {
  model_->Bind(column, value);
}

void Statement::Bind(unsigned column, int value) {
  model_->Bind(column, value);
}

void Statement::Bind(unsigned column, int64_t value) {
  model_->Bind(column, value);
}

void Statement::Bind(unsigned column, double value) {
  model_->Bind(column, value);
}

void Statement::Bind(unsigned column, const char* value) {
  model_->Bind(column, value);
}

void Statement::Bind(unsigned column, std::string_view value) {
  model_->Bind(column, value);
}

void Statement::Bind(unsigned column, std::u16string_view value) {
  model_->Bind(column, value);
}

size_t Statement::GetColumnCount() const {
  return model_->GetColumnCount();
}

ColumnType Statement::GetColumnType(unsigned column) const {
  return model_->GetColumnType(column);
}

bool Statement::GetColumnBool(unsigned column) const {
  return model_->GetColumnBool(column);
}

int Statement::GetColumnInt(unsigned column) const {
  return model_->GetColumnInt(column);
}

int64_t Statement::GetColumnInt64(unsigned column) const {
  return model_->GetColumnInt64(column);
}

double Statement::GetColumnDouble(unsigned column) const {
  return model_->GetColumnDouble(column);
}

std::string Statement::GetColumnString(unsigned column) const {
  return model_->GetColumnString(column);
}

std::u16string Statement::GetColumnString16(unsigned column) const {
  return model_->GetColumnString16(column);
}

void Statement::Run() {
  model_->Run();
}

bool Statement::Step() {
  return model_->Step();
}

void Statement::Reset() {
  model_->Reset();
}

void Statement::Close() {
  if (model_) {
    model_->Close();
    model_.reset();
  }
}

}  // namespace sql
