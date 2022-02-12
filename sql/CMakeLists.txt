find_package(unofficial-sqlite3 CONFIG REQUIRED)

file(GLOB sql_sources
  "*.cpp"
  "*.h"
)

add_library(sql STATIC ${sql_sources})

target_include_directories(sql PUBLIC "..")

target_link_libraries(sql PUBLIC base)

target_link_libraries(sql PUBLIC unofficial::sqlite3::sqlite3)

target_compile_definitions(sql PRIVATE -DSQLITE_THREADSAFE=0)

# Uses std::filesystem.
target_compile_features(sql PUBLIC cxx_std_17)

if(NOT WIN32)
  target_link_libraries(sql PUBLIC dl)
endif()