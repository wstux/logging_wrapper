LibTarget(logging_wrapper STATIC
    HEADERS
        logging_manager.h
        severity_level.h
        details/loggingc_defs.h
        details/loggingf_defs.h
    SOURCES
        details/logging_manager.cpp
    INCLUDE_DIR libs
)

