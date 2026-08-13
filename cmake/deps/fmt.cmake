include(FetchContent)
FetchContent_Declare(
    fmt
    # Specify the commit you depend on and update it regularly.
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG 12.2.0
    SYSTEM
)
FetchContent_MakeAvailable(fmt)
