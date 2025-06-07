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

