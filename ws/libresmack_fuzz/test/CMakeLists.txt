include(${CMAKE_SOURCE_DIR}/cmake/gtest.cmake)
enable_testing()

file(GLOB TEST_SOURCES *.cpp)

foreach(TEST_FILE IN LISTS TEST_SOURCES)
  cmake_path(GET TEST_FILE STEM TEST_NAME)
  set(CURR_TEST_NAME "test_${PROJECT_NAME}_${TEST_NAME}")

  add_executable("${CURR_TEST_NAME}"
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${TEST_FILE}"
  )

  target_link_libraries("${CURR_TEST_NAME}"
    GTest::gtest_main
    ${LIBRESMACK_FUZZ_SHARED}
  )
  target_compile_options(
    "${CURR_TEST_NAME}"
      PRIVATE
        -fno-omit-frame-pointer
        # -fsanitize-coverage=trace-pc-guard
  )

  gtest_discover_tests(
    "${CURR_TEST_NAME}"
  )
endforeach()
