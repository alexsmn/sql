#pragma once

#include "sql/postgresql/result.h"
#include "sql/types.h"

#include <cstdint>
#include <string>

namespace sql::postgresql {

class FieldView {
 public:
  FieldView(const Result& result, int field_index);

  ColumnType GetType() const;

  bool GetBool() const;
  int GetInt() const;
  std::int64_t GetInt64() const;
  double GetDouble() const;
  std::string GetString() const;
  std::u16string GetString16() const;

 private:
  const Result& result_;
  int field_index_;
};

}  // namespace sql::postgresql