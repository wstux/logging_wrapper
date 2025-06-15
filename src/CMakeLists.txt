add_subdirectory(libs/logging_wrapper)
add_subdirectory(libs/loggingf_wrapper)

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()

