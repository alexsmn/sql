file(GLOB SOURCES CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
)

add_library(sql_postgresql ${SOURCES})

target_include_directories(sql_postgresql PUBLIC "../..")

# Uses std::format.
target_compile_features(sql_postgresql PUBLIC cxx_std_20)

find_package(PostgreSQL REQUIRED)
target_link_libraries(sql_postgresql PUBLIC PostgreSQL::PostgreSQL)

find_package(Boost COMPONENTS locale REQUIRED)
target_link_libraries(sql_postgresql PUBLIC Boost::locale)
