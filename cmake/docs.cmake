# The MIT License
#
# Copyright (c) 2025 wstux
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

################################################################################
# Doxygen
################################################################################

find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(Doxygen_FOUND)
    set(_in_dirs "")
    if (EXISTS "${PROJECT_SOURCE_DIR}/README.md")
    #    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${PROJECT_SOURCE_DIR}/README.md")
    endif()

    set(_working_dir "${CMAKE_BINARY_DIR}/docs/doxygen")
    execute_process(COMMAND bash -c "mkdir -p ${_working_dir}")

    set(DOXYGEN_QUIET               YES)
    set(DOXYGEN_CALLER_GRAPH        YES)
    set(DOXYGEN_CALL_GRAPH          YES)
    set(DOXYGEN_EXTRACT_ALL         YES)
    set(DOXYGEN_GENERATE_TREEVIEW   YES)
    set(DOXYGEN_DOT_IMAGE_FORMAT    svg)
    set(DOXYGEN_DOT_TRANSPARENT     YES)
    set(DOXYGEN_GENERATE_HTML       YES)
    set(DOXYGEN_GENERATE_MAN        NO)
    set(DOXYGEN_OUTPUT_DIRECTORY    "${_working_dir}")

    set(_in_dirs "")
    #if (EXISTS "${PROJECT_SOURCE_DIR}/README.md")
    #    set(_in_dirs ${_in_dirs} "${PROJECT_SOURCE_DIR}/README.md")
    #endif()
    if (EXISTS "${PROJECT_SOURCE_DIR}/drivers")
        set(_in_dirs ${_in_dirs} "${PROJECT_SOURCE_DIR}/drivers")
    endif()
    if (EXISTS "${PROJECT_SOURCE_DIR}/include")
        set(_in_dirs ${_in_dirs} "${PROJECT_SOURCE_DIR}/include")
    endif()
    if (EXISTS "${PROJECT_SOURCE_DIR}/src")
        set(_in_dirs ${_in_dirs} "${PROJECT_SOURCE_DIR}/src")
    endif()

    #execute_process(COMMAND bash -c "mkdir -p ${_working_dir}")

    doxygen_add_docs(doxygen_docs ALL ${_in_dirs}
        WORKING_DIRECTORY "${_working_dir}"
        COMMENT "Generating documentation: ${_working_dir}/html/index.html")
endif()

