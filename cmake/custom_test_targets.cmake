# The MIT License
#
# Copyright (c) 2023 wstux
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

include(build_utils)

################################################################################
# Keywords
################################################################################

set(_CUSTOM_TEST_FLAGS_KW   )
set(_CUSTOM_TEST_VALUES_KW  INTERPRETER
                            SOURCE
)
set(_CUSTOM_TEST_LISTS_KW   )

################################################################################
# Targets
################################################################################

macro(CustomTestTarget TARGET_NAME)
    _parse_target_args(${TARGET_NAME}
        _CUSTOM_TEST_FLAGS_KW _CUSTOM_TEST_VALUES_KW _CUSTOM_TEST_LISTS_KW ${ARGN}
    )

    FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${${TARGET_NAME}_SOURCE}
         "${${TARGET_NAME}_INTERPRETER} ${CMAKE_CURRENT_SOURCE_DIR}/${${TARGET_NAME}_SOURCE}\n"
    )

    CustomTarget(${TARGET_NAME}
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${${TARGET_NAME}_SOURCE}
    )

    add_test(
        NAME ${TARGET_NAME}
        COMMAND sh ${CMAKE_CURRENT_BINARY_DIR}/${${TARGET_NAME}_SOURCE}
    )
endmacro()

