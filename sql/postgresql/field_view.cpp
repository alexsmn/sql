#include "sql/postgresql/field_view.h"

#include "sql/exception.h"
#include "sql/postgresql/conversions.h"

#include <boost/locale/encoding_utf.hpp>
#include <cassert>
#include <catalog/pg_type_d.h>
#include <span>

namespace sql::postgresql {

FieldView::FieldView(const Result& result, int field_index)
    : result_{result}, field_index_{field_index} {
  assert(result_);
  assert(field_index_ >= 0);
  assert(result_.field_count() > field_index_);
  assert(result_.field_format(field_index_) == 1);
}

ColumnType FieldView::GetType() const {
  if (result_.is_null(field_index_)) {
    return COLUMN_TYPE_NULL;
  }

  Oid type = result_.field_type(field_index_);
  switch (type) {
    case BOOLOID:
    case INT4OID:
    case INT8OID:
      return COLUMN_TYPE_INTEGER;
    case FLOAT4OID:
    case FLOAT8OID:
      return COLUMN_TYPE_FLOAT;
    case TEXTOID:
      return COLUMN_TYPE_TEXT;
    default:
      assert(false);
      return COLUMN_TYPE_NULL;
  }
}

bool FieldView::GetBool() const {
  return GetInt64() != 0;
}

int FieldView::GetInt() const {
  auto result64 = GetInt64();
  return static_cast<int>(result64) == result64 ? static_cast<int>(result64)
                                                : 0;
}

int64_t FieldView::GetInt64() const {
  return GetBufferInt64(result_.field_type(field_index_),
                        result_.value(field_index_));
}

double FieldView::GetDouble() const {
  return GetBufferDouble(result_.field_type(field_index_),
                         result_.value(field_index_));
}

std::string FieldView::GetString() const {
  auto buffer = result_.value(field_index_);
  return std::string{buffer.begin(), buffer.end()};
}

std::u16string FieldView::GetString16() const {
  std::string string = GetString();
  return boost::locale::conv::utf_to_utf<char16_t>(string);
}

}  // namespace sql::postgresql
