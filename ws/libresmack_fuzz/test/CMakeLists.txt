include(${CMAKE_SOURCE_DIR}/gtest.cmake)
include(GoogleTest)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/test
)

file(GLOB TESTS *.cpp)
add_executable(test_${PROJECT_NAME}
  ./main.cpp
  ${TESTS}
)

target_link_libraries(test_${PROJECT_NAME}
  libresmack
  libresmack_fuzz
  gtest
  pthread
  crypto
  unwind-ptrace
  unwind-generic
)

gtest_discover_tests(
    test_${PROJECT_NAME}
)
