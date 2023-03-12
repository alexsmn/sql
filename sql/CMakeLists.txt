file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
)

file(GLOB_RECURSE UT_SOURCES CONFIGURE_DEPENDS
  "*_unittest.*"
)

list(REMOVE_ITEM SOURCES ${UT_SOURCES})

add_library(sql STATIC ${SOURCES})

target_include_directories(sql PUBLIC "..")

find_package(unofficial-sqlite3 CONFIG REQUIRED)
target_link_libraries(sql PUBLIC unofficial::sqlite3::sqlite3)

find_package(Boost COMPONENTS locale REQUIRED)
target_link_libraries(sql PUBLIC Boost::locale)

target_compile_definitions(sql PRIVATE -DSQLITE_THREADSAFE=0)

# Uses std::format.
target_compile_features(sql PUBLIC cxx_std_20)

if(NOT WIN32)
  target_link_libraries(sql PUBLIC dl)
endif()

# UTs

add_executable(sql_unittests ${UT_SOURCES})
find_package(GTest REQUIRED)
target_link_libraries(sql_unittests PUBLIC sql GTest::gmock_main)
include(GoogleTest)
gtest_discover_tests(sql_unittests)
