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

cmake_minimum_required(VERSION 3.3)

project(libzlib)

set(ROOT ${PROJECT_SOURCE_DIR}/../../..)

include(${ROOT}/Config.cmake)

set(
  ZLIB_SRC

	adler32.c
	compress.c
	crc32.c
	deflate.c
	gzclose.c
	gzlib.c
	gzread.c
	gzwrite.c
	infback.c
	inffast.c
	inftrees.c
	trees.c
	uncompr.c
	zutil.c
)


if ((${TARGET_ARCH_ABI} STREQUAL x86_64) OR (${TARGET_ARCH_ABI} STREQUAL x86))
  set(
    ZLIB_SRC ${ZLIB_SRC}
    
    inflate.c
    x86.c
    adler32_simd.c
    crc_folding.c
    fill_window_sse.c
  )

  if (NOT DEFINED WINDOWS)
    add_definitions(-Wno-incompatible-pointer-types -mssse3 -msse4.2 -mpclmul)
  endif (NOT DEFINED WINDOWS)

  add_definitions(-DADLER32_SIMD_SSSE3)
endif ((${TARGET_ARCH_ABI} STREQUAL x86_64) OR (${TARGET_ARCH_ABI} STREQUAL x86))


if ((${TARGET_ARCH_ABI} STREQUAL armeabi) OR (${TARGET_ARCH_ABI} STREQUAL armeabi-v7a))
  set(
    ZLIB_SRC ${ZLIB_SRC}

    inflate.c
    simd_stub.c
  )
endif ((${TARGET_ARCH_ABI} STREQUAL armeabi) OR (${TARGET_ARCH_ABI} STREQUAL armeabi-v7a))


if (${TARGET_ARCH_ABI} STREQUAL arm64-v8a)
  set(
    ZLIB_SRC ${ZLIB_SRC}

    adler32_simd.c
    simd_stub.c
    contrib/optimizations/inffast_chunky.c
    contrib/optimizations/inflate.c
  )

  add_definitions(-DADLER32_SIMD_NEON)
  include_directories(contrib/optimizations/arm)
endif (${TARGET_ARCH_ABI} STREQUAL arm64-v8a)


if ((${TARGET_ARCH_ABI} STREQUAL mips) OR (${TARGET_ARCH_ABI} STREQUAL mips64))
  set(
    ZLIB_SRC ${ZLIB_SRC}

    inflate.c
    simd_stub.c
  )
endif ((${TARGET_ARCH_ABI} STREQUAL mips) OR (${TARGET_ARCH_ABI} STREQUAL mips64))


add_library(zlib STATIC ${ZLIB_SRC})


set_target_properties(zlib PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN 1)
set_target_properties(zlib PROPERTIES C_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN 1)
set_target_properties(zlib PROPERTIES ASM_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN 1)
set_property(TARGET zlib PROPERTY POSITION_INDEPENDENT ON)

if (DEFINED WINDOWS)
	add_definitions(-DV8_DEPRECATION_WARNINGS -DNO_TCMALLOC -D_HAS_EXCEPTIONS=0 -D__STD_C -D_CRT_RAND_S -D_CRT_SECURE_NO_DEPRECATE -D_SCL_SECURE_NO_DEPRECATE)
	add_definitions(-D_CRT_NONSTDC_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
	add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_UNICODE -DUNICODE)
	add_definitions(/wd4819 /wd4267 /wd4244)
endif (DEFINED WINDOWS)


