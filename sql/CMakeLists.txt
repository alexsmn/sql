file(GLOB SOURCES CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
)

file(GLOB UT_SOURCES CONFIGURE_DEPENDS
  "*_unittest.*"
)

list(REMOVE_ITEM SOURCES ${UT_SOURCES})

add_library(sql ${SOURCES})

target_include_directories(sql PUBLIC "..")

# Uses std::filesystem.
target_compile_features(sql PUBLIC cxx_std_17)

add_subdirectory(sqlite3)
target_link_libraries(sql PRIVATE sql_sqlite3)

add_subdirectory(postgresql)
target_link_libraries(sql PRIVATE sql_postgresql)

# UTs

add_executable(sql_unittests ${UT_SOURCES})
# Uses designated initializers.
target_compile_features(sql PUBLIC cxx_std_20)
find_package(GTest REQUIRED)
target_link_libraries(sql_unittests PUBLIC sql GTest::gmock_main)
include(GoogleTest)
gtest_discover_tests(sql_unittests)
