#include "sql/postgresql/field_view.h"

#include "sql/exception.h"
#include "sql/postgresql/conversions.h"

#include <boost/locale/encoding_utf.hpp>
#include <cassert>
#include <catalog/pg_type_d.h>
#include <span>

namespace sql::postgresql {

field_view::field_view(const result& result, int field_index)
    : result_{result}, field_index_{field_index} {
  assert(result_);
  assert(field_index_ >= 0);
  assert(result_.field_count() > field_index_);
  assert(result_.field_format(field_index_) == 1);
}

field_type field_view::type() const {
  if (result_.is_null(field_index_)) {
    return field_type::EMPTY;
  }

  Oid type = result_.field_type(field_index_);
  switch (type) {
    case BOOLOID:
    case INT4OID:
    case INT8OID:
      return field_type::INTEGER;
    case FLOAT4OID:
    case FLOAT8OID:
      return field_type::FLOAT;
    case TEXTOID:
      return field_type::TEXT;
    default:
      assert(false);
      return field_type::EMPTY;
  }
}

bool field_view::as_bool() const {
  return as_int64() != 0;
}

int field_view::as_int() const {
  auto result64 = as_int64();
  return static_cast<int>(result64) == result64 ? static_cast<int>(result64)
                                                : 0;
}

int64_t field_view::as_int64() const {
  if (result_.is_null(field_index_)) {
    return 0;
  }

  return GetBufferInt64(result_.field_type(field_index_),
                        result_.value(field_index_));
}

double field_view::as_double() const {
  if (result_.is_null(field_index_)) {
    return 0;
  }

  return GetBufferDouble(result_.field_type(field_index_),
                         result_.value(field_index_));
}

std::string field_view::as_string() const {
  if (result_.is_null(field_index_)) {
    return {};
  }

  auto buffer = result_.value(field_index_);
  return std::string{buffer.begin(), buffer.end()};
}

std::u16string field_view::as_string16() const {
  std::string string = as_string();
  return boost::locale::conv::utf_to_utf<char16_t>(string);
}

}  // namespace sql::postgresql
