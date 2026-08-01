# sql

A small C++ SQL database library presenting one API over SQLite3 and
PostgreSQL, with prepared statements, transactions, and schema introspection.

The backend is chosen at runtime from a driver string, so the same code runs
against either database. When you do not need that, the backend classes are
usable directly and skip the type erasure entirely.

## Features

* One API over SQLite3 and PostgreSQL
* Runtime backend selection, or direct use of a concrete backend
* Prepared statements with parameter binding and typed column access
* Transactions (`start` / `commit` / `rollback`)
* Schema introspection: table, column, and index existence plus column listing
* `std::string_view` / `std::u16string_view` parameters, UTF-8 and UTF-16 reads

## Usage

```c++
#include "sql/connection.h"
#include "sql/statement.h"

sql::connection connection{{
    .driver = "sqlite",
    .path = "database.sqlite3",
}};

connection.query("CREATE TABLE IF NOT EXISTS people (id INTEGER, name TEXT)");

sql::statement insert{connection, "INSERT INTO people (id, name) VALUES (?, ?)"};
insert.bind(0, 1);
insert.bind(1, "Alice");
insert.query();

sql::statement select{connection, "SELECT id, name FROM people"};
while (select.next()) {
  int id = select.at(0).as_int();
  std::string name = select.at(1).as_string();
}
```

**Column indices are 0-based**, for both `bind()` and `at()` — the SQLite
backend adds the +1 its C API wants.

### Transactions

```c++
connection.start();
try {
  // ... statements ...
  connection.commit();
} catch (const std::exception&) {
  connection.rollback();
  throw;
}
```

### Schema introspection

```c++
if (!connection.table_exists("people")) { /* ... */ }
if (!connection.field_exists("people", "email")) { /* ... */ }
if (!connection.index_exists("people", "people_id_idx")) { /* ... */ }

for (const sql::field_info& field : connection.table_fields("people")) {
  // field.name, field.type
}
```

## Backends

`open_params::driver` selects the backend:

| `driver` | Backend |
| --- | --- |
| `""` (empty), `"sqlite"`, `"sqlite3"` | SQLite3 |
| `"postgres"`, `"postgresql"` | PostgreSQL |

Any other value throws. `"postgres"` also throws when the library was built
with `SQL_ENABLE_POSTGRESQL=OFF`.

`open_params` carries both a `path` (SQLite file) and a `connection_string`
(PostgreSQL), plus the SQLite tuning knobs `exclusive_locking`,
`multithreaded`, and `journal_size_limit`:

```c++
sql::connection sqlite{{.driver = "sqlite",
                        .path = "database.sqlite3",
                        .exclusive_locking = true}};

sql::connection postgres{{.driver = "postgres",
                          .connection_string = "host=localhost dbname=test "
                                               "user=postgres"}};
```

### Using a backend directly

`sql::connection` and `sql::statement` dispatch through a virtual interface.
To avoid it, use a backend's own types — they expose the same member
functions, so code can be templated over either:

```c++
#include "sql/sqlite3/connection.h"
#include "sql/sqlite3/statement.h"

sql::sqlite3::connection connection;
connection.open({.path = "database.sqlite3"});
sql::sqlite3::statement statement{connection, "SELECT id FROM people"};
```

## Errors

The API reports failures by throwing; there is no status-returning variant.

Backend errors — a failed open, a bad statement, a failed step — throw
`sql::Exception` (`sql/exception.h`), which derives from
`std::runtime_error`. Driver selection in `connection::open()` throws plain
`std::runtime_error` for an unknown driver or a PostgreSQL request in a build
configured with `SQL_ENABLE_POSTGRESQL=OFF`, so catch `std::runtime_error`
rather than `sql::Exception` if you want to handle both.

## Dependencies

Declared in `vcpkg.json`:

* `sqlite3`
* `libpq` (PostgreSQL backend)
* `boost-endian`, `boost-locale`
* `gtest` (unit tests)

## Building

Requires **C++17** (the library uses `std::filesystem`). The unit tests
additionally require **C++20** for designated initializers.

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

### CMake options

| Option | Default | Effect |
| --- | --- | --- |
| `SQL_ENABLE_POSTGRESQL` | `ON` | Build the PostgreSQL backend and link `libpq` |

### Consuming from another project

Put this directory on `CMAKE_MODULE_PATH`, then:

```cmake
find_package(Sql REQUIRED)

target_link_libraries(my_target PRIVATE sql)
```

The library target is `sql`, also aliased as `Sql::sql`.

## Tests

Unit tests are built as `sql_unittests` and registered with CTest:

```sh
ctest --test-dir build
```

The test suite runs the same cases against both backends. The PostgreSQL
cases expect a reachable server — see the connection string in
`sql/connection_unittest.cpp`; without one, those cases fail rather than skip.

## License

MIT — see [LICENSE](LICENSE).
