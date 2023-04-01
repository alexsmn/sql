#pragma once

#include <boost/container/small_vector.hpp>
#include <boost/endian/conversion.hpp>
#include <catalog/pg_type_d.h>

namespace sql::postgresql {

template <class T>
inline T GetBuffer(std::span<const char> buffer) {
  if (buffer.size() != sizeof(T)) {
    throw Exception{"Unexpected value size"};
  }

  T result;
  memcpy(&result, buffer.data(), sizeof(result));
  return result;
}

template <class T>
inline void SetBuffer(boost::container::small_vector<char, 8>& buffer,
                      const T& value) {
  buffer.resize(sizeof(T));
  memcpy(buffer.data(), &value, sizeof(T));
}

inline int64_t GetBufferInt64(Oid type, std::span<const char> buffer) {
  switch (type) {
    case INT2OID:
      return boost::endian::native_to_big(GetBuffer<int16_t>(buffer));
    case INT4OID:
      return boost::endian::native_to_big(GetBuffer<int32_t>(buffer));
    case INT8OID:
      return boost::endian::native_to_big(GetBuffer<int64_t>(buffer));
    default:
      assert(false);
      return 0;
  }
}

inline void SetBufferValue(int64_t value,
                           Oid type,
                           boost::container::small_vector<char, 8>& buffer) {
  switch (type) {
    case BOOLOID:
      // TODO: Validate value narrowing.
      SetBuffer(buffer, boost::endian::native_to_big(
                            static_cast<int32_t>(value ? 1 : 0)));
      return;
    case INT4OID:
      // TODO: Validate value narrowing.
      SetBuffer(buffer,
                boost::endian::native_to_big(static_cast<int32_t>(value)));
      return;
    case INT8OID:
      SetBuffer(buffer, boost::endian::native_to_big(value));
      return;
    default:
      assert(false);
  }
}

inline double GetBufferDouble(Oid type, std::span<const char> buffer) {
  switch (type) {
    case FLOAT4OID:
      return GetBuffer<float>(buffer);
    case FLOAT8OID:
      return GetBuffer<double>(buffer);
    default:
      assert(false);
      return 0;
  }
}

inline void SetBufferValue(double value,
                           Oid type,
                           boost::container::small_vector<char, 8>& buffer) {
  switch (type) {
    case FLOAT4OID:
      // TODO: Validate value narrowing.
      SetBuffer(buffer, static_cast<float>(value));
      return;
    case FLOAT8OID:
      SetBuffer(buffer, value);
      return;
    default:
      assert(false);
  }
}

inline void SetBufferValue(std::string_view str,
                           Oid type,
                           boost::container::small_vector<char, 8>& buffer) {
  switch (type) {
    case NAMEOID:
    case TEXTOID:
      buffer.assign(str.begin(), str.end());
      return;
    default:
      assert(false);
  }
}

}  // namespace sql::postgresql
