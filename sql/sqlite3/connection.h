#pragma once

#include "sql/types.h"

#include <memory>
#include <string>
#include <vector>

struct sqlite3;

namespace sql {
struct open_params;
}

namespace sql::sqlite3 {

class statement;

class connection {
 public:
  using statement = sql::sqlite3::statement;

  connection() = default;
  explicit connection(const open_params& params);
  ~connection();

  connection(const connection&) = delete;
  connection& operator=(const connection&) = delete;

  void open(const open_params& params);
  void close();

  void query(std::string_view sql);

  void start();
  void commit();
  void rollback();

  int last_change_count() const;

  bool table_exists(std::string_view table_name) const;
  bool field_exists(std::string_view table_name,
                    std::string_view field_name) const;
  bool index_exists(std::string_view table_name,
                    std::string_view index_name) const;

  std::vector<field_info> table_fields(std::string_view table_name) const;

 private:
  ::sqlite3* db_ = nullptr;

  mutable std::unique_ptr<statement> begin_transaction_statement_;
  mutable std::unique_ptr<statement> commit_transaction_statement_;
  mutable std::unique_ptr<statement> rollback_transaction_statement_;

  mutable std::unique_ptr<statement> does_table_exist_statement_;
  mutable std::unique_ptr<statement> does_column_exist_statement_;
  mutable std::unique_ptr<statement> does_index_exist_statement_;
  mutable std::string does_column_exist_table_name_;
  mutable std::string does_index_exist_table_name_;

  // Avoid conflicts with the local `using statement`.
  friend class sql::sqlite3::statement;
};

}  // namespace sql::sqlite3
