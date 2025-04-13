# Unit tests

TestTarget(ut_logging_wrapper
    SOURCES
        ut_logging_wrapper.cpp
    LIBRARIES
        logging_wrapper
    DEPENDS
        testing
)

TestTarget(ut_loggingf_wrapper
    SOURCES
        ut_loggingf_wrapper.cpp
    LIBRARIES
        loggingf_wrapper
    DEPENDS
        testing
)

