# The MIT License
#
# Copyright (c) 2022 wstux
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

include(ExternalProject)

include(build_utils)

################################################################################
# Setting of cmake policies
################################################################################

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
	cmake_policy(SET CMP0135 NEW)
endif()

################################################################################
# Constants
################################################################################

set(EXTERNALS_PREFIX "${CMAKE_BINARY_DIR}/externals")

################################################################################
# Keywords
################################################################################

set(_EXT_TARGET_FLAGS_KW    )
set(_EXT_TARGET_VALUES_KW   BUILD_COMMAND
                            CONFIGURE_COMMAND
                            INCLUDE_DIR
                            INSTALL_COMMAND
                            INSTALL_DIR
                            LIBRARIES
                            URL
                            #URL_MD5
)
set(_EXT_TARGET_LISTS_KW    #DEPENDS
                            #LIBRARIES
)

################################################################################
# Utility functiona
################################################################################

function(_set_command VAR VALUE DFL_VALUE)
    set(${VAR} "true" PARENT_SCOPE)
    if (${VALUE})
        set(${VAR} bash -c "${${VALUE}}" PARENT_SCOPE)
    endif()
endfunction()

################################################################################
# Targets
################################################################################

function(ExternalTarget EXT_TARGET_NAME)
    _parse_target_args_strings(${EXT_TARGET_NAME}
        _EXT_TARGET_FLAGS_KW _EXT_TARGET_VALUES_KW _EXT_TARGET_LISTS_KW ${ARGN}
    )

    set(_target_name    "${EXT_TARGET_NAME}")
    set(_target_arch    "${_target_name}.tar.gz")
    set(_target_dir     "${EXTERNALS_PREFIX}/${_target_name}")

    set(_ext_url        ${${EXT_TARGET_NAME}_URL})
    set(_ext_url_hash   ${${EXT_TARGET_NAME}_URL_MD5})

    _set_command(_configure_cmd ${EXT_TARGET_NAME}_CONFIGURE_COMMAND "true")
    _set_command(_build_cmd     ${EXT_TARGET_NAME}_BUILD_COMMAND     "true")

    set(_include_dir "${${EXT_TARGET_NAME}_INCLUDE_DIR}")
    set(_install_dir "${${EXT_TARGET_NAME}_INSTALL_DIR}")
    _set_command(_install_command ${EXT_TARGET_NAME}_INSTALL_COMMAND "true")

    set(_depends "")

    ExternalProject_Add(${_target_name}
        PREFIX              ${_target_dir}
        STAMP_DIR           ${_target_dir}/stamp
        TMP_DIR             ${_target_dir}/tmp
        URL                 ${_ext_url}
        URL_MD5             ${_ext_url_hash}
        CONFIGURE_COMMAND   ${_configure_cmd}
        BUILD_IN_SOURCE     1
        INSTALL_COMMAND     ${_install_command}
        INSTALL_DIR         ${_install_dir}
        BUILD_COMMAND       ${_build_cmd}
        DEPENDS ${_depends}
    )

    set(_libraries "")
    if (${EXT_TARGET_NAME}_LIBRARIES)
        foreach (_lib IN LISTS ${EXT_TARGET_NAME}_LIBRARIES)
            set(_libraries "${_libraries}" "${_install_dir}/lib/${_lib}")
        endforeach()
    endif()

    set_target_properties(${EXT_TARGET_NAME} PROPERTIES INCLUDE_DIRECTORIES "${_include_dir}")
    set_target_properties(${EXT_TARGET_NAME} PROPERTIES IMPORTED_LOCATION "${_libraries}")
    set_target_properties(${EXT_TARGET_NAME} PROPERTIES INSTALL_DIR "${install_dir}")
endfunction()

