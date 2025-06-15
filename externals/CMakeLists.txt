if (BUILD_EXAMPLES)
    ExternalTarget(boost
        URL
            https://github.com/boostorg/boost/releases/download/boost-1.88.0/boost-1.88.0-cmake.tar.gz
        #URL_MD5
        #    fe33be48eeb6de4dc2fefe60a3834c1f
        CONFIGURE_COMMAND
            ./bootstrap.sh --prefix=${EXTERNALS_PREFIX}/boost/install --with-toolset=gcc --with-libraries=date_time,filesystem,log,thread
        INSTALL_COMMAND
            ./b2 linkflags=\"${CMAKE_EXE_LINKER_FLAGS}\" cflags=\"${CMAKE_C_FLAGS}\" cxxflags=\"${CMAKE_CXX_FLAGS}\" architecture=x86 address-model=64 link=shared,static --debug-configuration -q --prefix=${EXTERNALS_PREFIX}/boost/install --layout=tagged -j8 threading=multi runtime-link=shared define=BOOST_LOG_USE_NATIVE_SYSLOG variant=release install
        INCLUDE_DIR
            ${EXTERNALS_PREFIX}/boost/install/include
        INSTALL_DIR
            ${EXTERNALS_PREFIX}/boost/install
        LIBRARIES
            libboost_date_time-mt-x64.so
            libboost_filesystem-mt-x64.a
            libboost_filesystem-mt-x64.so
            libboost_log-mt-x64.so
            libboost_log_setup-mt-x64.so
            libboost_thread-mt-x64.so
    )
endif()

if (BUILD_TESTS)
    ExternalTarget(testing
        URL
            https://github.com/wstux/testing_template/archive/refs/heads/master.zip
        #URL_MD5
        #    fe33be48eeb6de4dc2fefe60a3834c1f
        CONFIGURE_COMMAND
            cmake --install-prefix ${EXTERNALS_PREFIX}/testing/install  ./
        BUILD_COMMAND
            cmake --build ./ && cmake --install ./
        INCLUDE_DIR
            ${EXTERNALS_PREFIX}/testing/install/include
        INSTALL_DIR
            ${EXTERNALS_PREFIX}/testing/install
    )
endif()

