sql_library(sql)

target_include_directories(sql PUBLIC "..")

# Uses std::filesystem.
target_compile_features(sql PUBLIC cxx_std_17)

add_subdirectory(sqlite3)
target_link_libraries(sql PRIVATE sql_sqlite3)

option(SQL_ENABLE_POSTGRESQL "Build PostgreSQL SQL backend" ON)
if(SQL_ENABLE_POSTGRESQL)
  add_subdirectory(postgresql)
  target_link_libraries(sql PRIVATE sql_postgresql)
  target_compile_definitions(sql PRIVATE SQL_ENABLE_POSTGRESQL=1)
endif()

# UTs

# Uses designated initializers.
target_compile_features(sql_unittests PUBLIC cxx_std_20)
