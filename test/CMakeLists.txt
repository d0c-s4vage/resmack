set(GTEST_LIB_DIR ${PROJECT_SOURCE_DIR}/libs-test/googletest/build/lib)
set(GTEST_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/libs-test/googletest/googletest/include)
set(GMOCK_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/libs-test/googletest/googlemock/include)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/test
    ${GTEST_INCLUDE_DIR}
    ${GMOCK_INCLUDE_DIR}
)

file(GLOB TESTS *.cpp)
add_executable(test_${PROJECT_NAME}
  ./main.cpp
  ${TESTS}
)

target_link_libraries(test_${PROJECT_NAME}
  ${RESMACK_LIB}
  ${GTEST_LIB_DIR}/libgtest.a
  pthread
)
