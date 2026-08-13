# see https://github.com/google/googletest/blob/main/googletest/README.md#incorporating-into-an-existing-cmake-project

include(FetchContent)
FetchContent_Declare(
  googletest
  # Specify the commit you depend on and update it regularly.
  GIT_REPOSITORY https://github.com/google/googletest
  GIT_TAG v1.17.0
  PREFIX gtest
  SYSTEM
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

target_compile_options(gtest PRIVATE $<$<CONFIG:Release>:-O0>)
target_compile_options(gtest_main PRIVATE $<$<CONFIG:Release>:-O0>)

# Add explicit debug macros if needed by GoogleTest
target_compile_definitions(gtest PRIVATE _DEBUG)
target_compile_definitions(gtest_main PRIVATE _DEBUG)
