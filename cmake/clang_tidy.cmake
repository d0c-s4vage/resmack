set(CMAKE_CXX_CLANG_TIDY
      "clang-tidy"
        "--quiet"
        "--use-color"
        "--header-filter=(${CMAKE_CURRENT_SOURCE_DIR}/(src|include)/.)|(${CMAKE_CURRENT_BINARY_DIR}/(src|include)/.)*"
)
