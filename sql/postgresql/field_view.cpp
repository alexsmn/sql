#include "sql/postgresql/field_view.h"

#include <boost/endian/conversion.hpp>
#include <boost/locale/encoding_utf.hpp>
#include <cassert>
#include <catalog/pg_type_d.h>

namespace sql::postgresql {

FieldView::FieldView(const Result& result, int field_index)
    : result_{result}, field_index_{field_index} {
  assert(result_);
  assert(field_index_ >= 0);
  assert(result_.field_count() > field_index_);
  assert(result_.field_format(field_index_) == 1);
}

template <class T>
T FieldView::GetValue() const {
  assert(result_.field_size(field_index_) == sizeof(T));

  if (result_.is_null(field_index_)) {
    return {};
  }

  auto* buffer = result_.value(field_index_);
  assert(buffer);

  T value;
  memcpy(&value, buffer, sizeof(T));
  return value;
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
  Oid type = result_.field_type(field_index_);
  switch (type) {
    case INT2OID:
      return boost::endian::native_to_big(GetValue<int16_t>());
    case INT4OID:
      return boost::endian::native_to_big(GetValue<int32_t>());
    case INT8OID:
      return boost::endian::native_to_big(GetValue<int64_t>());
    default:
      assert(false);
      return 0;
  }
}

double FieldView::GetDouble() const {
  Oid type = result_.field_type(field_index_);
  switch (type) {
    case FLOAT4OID:
      return GetValue<float>();
    case FLOAT8OID:
      return GetValue<double>();
    default:
      assert(false);
      return 0;
  }
}

std::string FieldView::GetString() const {
  assert(result_.field_format(field_index_) == 1);
  auto* buffer = result_.value(field_index_);
  return buffer ? std::string{buffer} : std::string{};
}

std::u16string FieldView::GetString16() const {
  std::string string = GetString();
  return boost::locale::conv::utf_to_utf<char16_t>(string);
}

}  // namespace sql::postgresql