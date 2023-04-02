#pragma once

#include "sql/types.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

typedef struct pg_conn PGconn;

namespace sql {
struct open_params;
}

namespace sql::postgresql {

class statement;

class connection {
 public:
  using statement = sql::postgresql::statement;

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
                    std::string_view column_name) const;
  bool index_exists(std::string_view table_name,
                    std::string_view index_name) const;

  std::vector<field_info> table_fields(std::string_view table_name) const;

 private:
  std::string GenerateStatementName();

  ::PGconn* conn_ = nullptr;

  mutable std::unique_ptr<statement> begin_transaction_statement_;
  mutable std::unique_ptr<statement> commit_transaction_statement_;
  mutable std::unique_ptr<statement> rollback_transaction_statement_;

  mutable std::unique_ptr<statement> table_columns_statement_;
  mutable std::unique_ptr<statement> does_table_exist_statement_;
  mutable std::unique_ptr<statement> does_column_exist_statement_;
  mutable std::unique_ptr<statement> does_index_exist_statement_;

  std::atomic<int> next_statement_id_ = 0;

  std::atomic<int> last_change_count_ = 0;

  friend class statement;
};

}  // namespace sql::postgresql
