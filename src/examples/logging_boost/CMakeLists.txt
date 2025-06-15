ExecTarget(logging_boost STATIC
    SOURCES
        main.cpp
    LIBRARIES
        logging_wrapper
    COMPILE_DEFINITIONS
        BOOST_LOG_DYN_LINK
    DEPENDS
        boost
        pthread
)

