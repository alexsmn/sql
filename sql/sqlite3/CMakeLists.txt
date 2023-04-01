sql_library(sql_sqlite3)

target_include_directories(sql_sqlite3 PUBLIC "../..")

# Uses std::format.
target_compile_features(sql_sqlite3 PUBLIC cxx_std_20)

# Uses Sqlite from Vcpkg.
find_package(unofficial-sqlite3 CONFIG REQUIRED)
target_link_libraries(sql_sqlite3 PUBLIC unofficial::sqlite3::sqlite3)
target_compile_definitions(sql_sqlite3 PUBLIC -DSQLITE_THREADSAFE=0)

find_package(Boost COMPONENTS locale REQUIRED)
target_link_libraries(sql_sqlite3 PUBLIC Boost::boost Boost::locale)

if(NOT WIN32)
  # target_link_libraries(sql_sqlite3 PUBLIC dl)
endif()
