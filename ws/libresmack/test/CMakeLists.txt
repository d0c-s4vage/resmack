include(${CMAKE_SOURCE_DIR}/cmake/gtest.cmake)
enable_testing()

file(GLOB TESTS *.cpp)
add_executable(test_${PROJECT_NAME}
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
  ${TESTS}
)

target_include_directories(
  test_${PROJECT_NAME}
  PRIVATE
    ${${PROJECT_NAME}_SOURCE_DIR}/src
)

target_link_libraries(test_${PROJECT_NAME}
  GTest::gtest_main
  ${LIBRESMACK_SHARED}
)

gtest_discover_tests(
    test_${PROJECT_NAME}
)
