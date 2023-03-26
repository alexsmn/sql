#pragma once

#include "sql/types.h"

#include <cstdint>
#include <string>

typedef struct pg_result PGresult;

namespace sql::postgresql {

class FieldView {
 public:
  FieldView(const PGresult* result, int field_index);

  ColumnType GetType() const;

  bool GetBool() const;
  int GetInt() const;
  std::int64_t GetInt64() const;
  double GetDouble() const;
  std::string GetString() const;
  std::u16string GetString16() const;

 private:
  template <class T>
  T GetValue() const;

  const PGresult* result_;
  int field_index_;
};

}  // namespace sql::postgresql