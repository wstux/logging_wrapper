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

include(build_utils)

function(Sanitizers)
    set(_name       Sanitizers)
    set(_flags_kw   ADDRESS LEAK UNDEFINED_BEHAVIOR THREAD)
    set(_values_kw  )
    set(_lists_kw   )
    _parse_target_args(${_name} _flags_kw _values_kw _lists_kw ${ARGN})

    set(_sanitizers "")

    if (${_name}_ADDRESS)
        list(APPEND _sanitizers "address")
    endif()

    if (${_name}_LEAK)
        list(APPEND _sanitizers "leak")
    endif()

    if (${_name}_UNDEFINED_BEHAVIOR)
        list(APPEND _sanitizers "undefined")
    endif()

    if (${_name}_THREAD)
        if (${_name}_ADDRESS OR ${_name}_LEAK)
            message(WARNING "Thread sanitizer does not work with address and leak sanitizer")
        else()
            list(APPEND _sanitizers "thread")
        endif()
    endif()

    list(JOIN _sanitizers "," _sanitizers)
    if (_sanitizers)
        #target_compile_options(${project_name} INTERFACE -fsanitize=${_sanitizers})
        #target_link_options(${project_name} INTERFACE -fsanitize=${_sanitizers})
        add_compile_options(-fsanitize=${_sanitizers})
        add_link_options(-fsanitize=${_sanitizers})
    endif()
endfunction()

