LibTarget(loggingf_wrapper STATIC
    HEADERS
        logging.h
        manager.h
        severity_level.h
    SOURCES
        details/manager.c
)

set_target_properties(loggingf_wrapper PROPERTIES LINKER_LANGUAGE C)

