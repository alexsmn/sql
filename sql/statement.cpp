#include "sql/statement.h"

#include "sql/exception.h"

#include <cassert>

namespace sql {

statement::statement(connection& connection, std::string_view sql) {
  prepare(connection, sql);
}

bool statement::is_prepared() const {
  return model_ && model_->is_prepared();
};

void statement::prepare(connection& connection, std::string_view sql) {
  model_ = connection.model_->create_statement_model(sql);
}

void statement::bind_null(unsigned column) {
  model_->bind_null(column);
}

void statement::bind(unsigned column, bool value) {
  model_->bind(column, value);
}

void statement::bind(unsigned column, int value) {
  model_->bind(column, value);
}

void statement::bind(unsigned column, int64_t value) {
  model_->bind(column, value);
}

void statement::bind(unsigned column, double value) {
  model_->bind(column, value);
}

void statement::bind(unsigned column, const char* value) {
  model_->bind(column, value);
}

void statement::bind(unsigned column, std::string_view value) {
  model_->bind(column, value);
}

void statement::bind(unsigned column, std::u16string_view value) {
  model_->bind(column, value);
}

size_t statement::field_count() const {
  return model_->field_count();
}

field_type statement::field_type(unsigned column) const {
  return model_->field_type(column);
}

field_view statement::at(unsigned column) const {
  assert(model_);
  return field_view{*model_, static_cast<int>(column)};
}

void statement::query() {
  model_->query();
}

bool statement::next() {
  return model_->next();
}

void statement::reset() {
  model_->reset();
}

void statement::close() {
  if (model_) {
    model_->close();
    model_.reset();
  }
}

}  // namespace sql
