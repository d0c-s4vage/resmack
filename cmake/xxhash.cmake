set(XXHASH_VERSION 0.8.3)

include(FetchContent)
FetchContent_Declare(
  xxhash
  GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
  GIT_TAG        v${XXHASH_VERSION}
  SOURCE_SUBDIR cmake_unofficial
)

set(XXH_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(XXH_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(XXH_STATIC_LINKING_ONLY ON CACHE BOOL "" FORCE)
set(XXH_BUILD_XXHSUM OFF CACHE BOOL "" FORCE)
set(XXH_BUILD_ENABLE_INLINE_API OFF CACHE BOOL "" FORCE)

# Ensure XXH3 is not disabled (it is enabled by default)
# set(XXH_NO_XXH3 OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(xxHash)
