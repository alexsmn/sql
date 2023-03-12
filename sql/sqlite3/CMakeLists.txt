file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
)

add_library(sql_sqlite3 ${SOURCES})

target_include_directories(sql_sqlite3 PUBLIC "../..")

# Uses std::format.
target_compile_features(sql_sqlite3 PUBLIC cxx_std_20)

find_package(unofficial-sqlite3 CONFIG REQUIRED)
target_link_libraries(sql_sqlite3 PUBLIC unofficial::sqlite3::sqlite3)

find_package(Boost COMPONENTS locale REQUIRED)
target_link_libraries(sql_sqlite3 PUBLIC Boost::locale)

target_compile_definitions(sql_sqlite3 PUBLIC -DSQLITE_THREADSAFE=0)

if(NOT WIN32)
  target_link_libraries(sql_sqlite3 PUBLIC dl)
endif()
