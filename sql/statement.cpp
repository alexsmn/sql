#include "sql/statement.h"

#include "sql/exception.h"

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

bool statement::get_bool(unsigned column) const {
  return model_->get_bool(column);
}

int statement::get_int(unsigned column) const {
  return model_->get_int(column);
}

int64_t statement::get_int64(unsigned column) const {
  return model_->get_int64(column);
}

double statement::get_double(unsigned column) const {
  return model_->get_double(column);
}

std::string statement::get_string(unsigned column) const {
  return model_->get_string(column);
}

std::u16string statement::get_string16(unsigned column) const {
  return model_->get_string16(column);
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
