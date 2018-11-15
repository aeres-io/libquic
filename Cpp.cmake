#
# MIT License
#
# Copyright (c) 2018 aeres.io
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# =============================================================================
#

macro(aeres_use_pthread target)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  target_link_libraries(${ARGV0} Threads::Threads)
endmacro(aeres_use_pthread)

macro(aeres_lib target name path)
  if (NOT TARGET ${ARGV1})
    add_library(${ARGV1} STATIC IMPORTED)
    set_target_properties(${ARGV1} PROPERTIES IMPORTED_LOCATION ${ARGV2})
  endif (NOT TARGET ${ARGV1})
  target_link_libraries(${ARGV0} ${ARGV1})
endmacro(aeres_lib)

macro(aeres_sys_lib target name)
  find_library(${ARGV1} ${ARGV1})
  target_link_libraries(${ARGV0} ${ARGV1})
endmacro(aeres_sys_lib)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT DEFINED TARGET_ARCH_ABI)
  if (NOT (WINDOWS OR WINDOWS_STORE OR WINDOWS_PHONE))
    set (UNAME_CMD "uname")
    set (UNAME_ARG "-m")
    execute_process(COMMAND ${UNAME_CMD} ${UNAME_ARG}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        RESULT_VARIABLE UNAME_RESULT
        OUTPUT_VARIABLE UNAME_MACHINE)

    # Use Regex; match i386, i486, i586 and i686
    if (NOT (${UNAME_MACHINE} MATCHES "i.86"))
      if (NOT (${UNAME_MACHINE} MATCHES "x86_64"))
        message(FATAL_ERROR, "Please define TARGET_ARCH_ABI to build.")
      else()
        set(TARGET_ARCH_ABI x86_64)
      endif (NOT (${UNAME_MACHINE} MATCHES "x86_64"))
    else()
      set(TARGET_ARCH_ABI x86)
    endif (NOT (${UNAME_MACHINE} MATCHES "i.86"))
  endif()
endif (NOT DEFINED TARGET_ARCH_ABI)

message("-- TARGET_ARCH_ABI set to ${TARGET_ARCH_ABI}")
