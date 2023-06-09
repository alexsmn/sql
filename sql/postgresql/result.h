#pragma once

#include <libpq-fe.h>
#include <libpq/libpq-fs.h>
#include <span>
#include <string>

namespace sql::postgresql {

class result {
 public:
  result() = default;
  explicit result(PGresult* result) : result_{result} {}
  ~result() { PQclear(result_); }

  result(const result&) = delete;
  result& operator=(const result&) = delete;

  result& operator=(result&& other) noexcept {
    if (result_ != other.result_) {
      PQclear(result_);
      result_ = other.result_;
      other.result_ = nullptr;
    }
    return *this;
  }

  explicit operator bool() const { return result_ != nullptr; }

  PGresult* get() { return result_; }
  const PGresult* get() const { return result_; }

  void reset() {
    PQclear(result_);
    result_ = nullptr;
  }

  void reset(PGresult* result) {
    if (result_ != result) {
      PQclear(result_);
      result_ = result;
    }
  }

  ExecStatusType status() const { return PQresultStatus(result_); }
  std::string_view error_message() const {
    return PQresultErrorMessage(result_);
  }

  bool is_null(int field_index) const {
    return PQgetisnull(result_, 0, field_index);
  }

  std::span<const char> value(int field_index) const {
    int size = PQgetlength(result_, 0, field_index);
    const char* data = PQgetvalue(result_, 0, field_index);
    return std::span<const char>{data, data + size};
  }

  int field_count() const { return PQnfields(result_); }
  std::string_view field_name(int index) const {
    return PQfname(result_, index);
  }
  int field_format(int index) const { return PQfformat(result_, index); }
  int field_type(int index) const { return PQftype(result_, index); }
  int field_size(int index) const { return PQfsize(result_, index); }

  int param_count() const { return PQnparams(result_); }
  Oid param_type(int index) const { return PQparamtype(result_, index); }

  int affected_row_count() const {
    const char* tuples = PQcmdTuples(result_);
    return tuples && *tuples ? std::stoi(tuples) : 0;
  }

 private:
  PGresult* result_ = nullptr;
};

}  // namespace sql::postgresql
