#pragma once

#include "sql/types.h"

#include <filesystem>
#include <vector>

namespace sql {

class statement;

class connection {
 public:
  using statement = sql::statement;

  connection() = default;
  explicit connection(const open_params& params);

  connection(const connection&) = delete;
  connection& operator=(const connection&) = delete;

  connection(connection&& source) noexcept : model_{std::move(source.model_)} {}
  connection& operator=(connection&& source) noexcept {
    model_ = std::move(source.model_);
    return *this;
  }

  void open(const open_params& params);
  void close() { model_->close(); }

  void query(std::string_view sql) { model_->query(sql); }

  void start() { model_->start(); }
  void commit() { model_->commit(); }
  void rollback() { model_->rollback(); }

  int last_change_count() const { return model_->last_change_count(); }

  bool table_exists(std::string_view table_name) const {
    return model_->table_exists(table_name);
  }
  bool field_exists(std::string_view table_name,
                    std::string_view column_name) const {
    return model_->field_exists(table_name, column_name);
  }
  bool index_exists(std::string_view table_name,
                    std::string_view index_name) const {
    return model_->index_exists(table_name, index_name);
  }

  std::vector<field_info> table_fields(std::string_view table_name) const {
    return model_->table_fields(table_name);
  }

 private:
  class statement_model {
   public:
    virtual ~statement_model() = default;

    virtual bool is_prepared() const = 0;

    virtual void bind_null(unsigned column) = 0;
    virtual void bind(unsigned column, bool value) = 0;
    virtual void bind(unsigned column, int value) = 0;
    virtual void bind(unsigned column, int64_t value) = 0;
    virtual void bind(unsigned column, double value) = 0;
    virtual void bind(unsigned column, const char* value) = 0;
    virtual void bind(unsigned column, const char16_t* value) = 0;
    virtual void bind(unsigned column, std::string_view value) = 0;
    virtual void bind(unsigned column, std::u16string_view value) = 0;

    virtual size_t field_count() const = 0;
    virtual field_type field_type(unsigned column) const = 0;

    virtual bool get_bool(unsigned column) const = 0;
    virtual int get_int(unsigned column) const = 0;
    virtual int64_t get_int64(unsigned column) const = 0;
    virtual double get_double(unsigned column) const = 0;
    virtual std::string get_string(unsigned column) const = 0;
    virtual std::u16string get_string16(unsigned column) const = 0;

    virtual void query() = 0;
    virtual bool next() = 0;
    virtual void reset() = 0;

    virtual void close() = 0;
  };

  class connection_model {
   public:
    virtual ~connection_model() = default;

    virtual void open(const open_params& params) = 0;
    virtual void close() = 0;

    virtual void query(std::string_view sql) = 0;

    virtual void start() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;

    virtual int last_change_count() const = 0;

    virtual bool table_exists(std::string_view table_name) const = 0;
    virtual bool field_exists(std::string_view table_name,
                              std::string_view column_name) const = 0;
    virtual bool index_exists(std::string_view table_name,
                              std::string_view index_name) const = 0;

    virtual std::vector<field_info> table_fields(
        std::string_view table_name) const = 0;

    virtual std::unique_ptr<statement_model> create_statement_model(
        std::string_view sql) = 0;
  };

  std::unique_ptr<connection_model> model_;

  template <class ConnectionType, class StatementType>
  class connection_model_impl;

  template <class ConnectionType, class StatementType>
  class statement_model_impl;

  friend class statement;
};

}  // namespace sql
