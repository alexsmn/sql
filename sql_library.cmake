find_package(GTest REQUIRED)
include(GoogleTest)

macro(sql_library TARGET_NAME)
  file(GLOB ${TARGET_NAME}_SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")
  file(GLOB ${TARGET_NAME}_UT_SOURCES CONFIGURE_DEPENDS "*_unittest.*" "*_mock.*")

  if (${TARGET_NAME}_UT_SOURCES)
    list(REMOVE_ITEM ${TARGET_NAME}_SOURCES ${${TARGET_NAME}_UT_SOURCES})
  endif()

  add_library(${TARGET_NAME} ${${TARGET_NAME}_SOURCES})
  add_library(Sql::${TARGET_NAME} ALIAS ${TARGET_NAME})

  if (${TARGET_NAME}_UT_SOURCES)
    add_executable(${TARGET_NAME}_unittests ${${TARGET_NAME}_UT_SOURCES})
    target_link_libraries(${TARGET_NAME}_unittests PUBLIC ${TARGET_NAME} GTest::gmock_main)
    gtest_discover_tests(${TARGET_NAME}_unittests)
  endif()
endmacro()
