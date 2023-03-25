sql_library(sql)

target_include_directories(sql PUBLIC "..")

# Uses std::filesystem.
target_compile_features(sql PUBLIC cxx_std_17)

add_subdirectory(sqlite3)
target_link_libraries(sql PRIVATE sql_sqlite3)

add_subdirectory(postgresql)
target_link_libraries(sql PRIVATE sql_postgresql)

# UTs

# Uses designated initializers.
target_compile_features(sql_unittests PUBLIC cxx_std_20)
