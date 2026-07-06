include(${CMAKE_SOURCE_DIR}/cmake/gtest.cmake)
enable_testing()

file(GLOB TEST_SOURCES *.cpp)

# add_executable("tester"
#   "${CMAKE_CURRENT_SOURCE_DIR}/tester.cpp"
#   # "${TEST_FILE}"
# )
#   target_link_libraries("tester"
#     # the release libraries aren't built with asan, so we need
#     # to explicitly add it for the tests
#     # GTest::gtest_main
#     ${LIBRESMACK_FUZZ_STATIC}
#     $<$<CONFIG:Release>:asan>
#     $<$<CONFIG:RelWithDebInfo>:asan>
#   )
#   target_compile_options(
#     "tester"
#       PRIVATE
#         -O0
#         -g
#         SHELL:${ASAN_FLAGS}
#         SHELL:${COVERAGE_FLAGS}
#   )

foreach(TEST_FILE IN LISTS TEST_SOURCES)
  cmake_path(GET TEST_FILE STEM TEST_NAME)
  set(CURR_TEST_NAME "test_${PROJECT_NAME}_${TEST_NAME}")

  add_executable("${CURR_TEST_NAME}"
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${TEST_FILE}"
  )

  target_link_libraries("${CURR_TEST_NAME}"
    # the release libraries aren't built with asan, so we need
    # to explicitly add it for the tests
    GTest::gtest_main
    ${LIBRESMACK_FUZZ_STATIC}
    $<$<CONFIG:Release>:asan>
    $<$<CONFIG:RelWithDebInfo>:asan>
  )
  target_compile_options(
    "${CURR_TEST_NAME}"
      PRIVATE
        -O0
        -g
        SHELL:${ASAN_FLAGS}
        SHELL:${COVERAGE_FLAGS}
  )

  gtest_discover_tests(
    "${CURR_TEST_NAME}"
  )
endforeach()
