# Copyright 2018 - Filegear - All Rights Reserved

FG_LIB := quic_zlib

FG_SRCS := \
	adler32.c \
	compress.c \
	crc32.c \
	deflate.c \
	gzclose.c \
	gzlib.c \
	gzread.c \
	gzwrite.c \
	infback.c \
	inffast.c \
	inftrees.c \
	trees.c \
	uncompr.c \
	zutil.c \

ifneq ($(findstring $(TARGET_ARCH_ABI), x86 x86_64),)
FG_SRCS += \
	inflate.c \
	x86.c \
	adler32_simd.c \
	crc_folding.c \
 	fill_window_sse.c \

FG_CFLAGS := -mssse3 -DADLER32_SIMD_SSSE3 -msse4.2 -mpclmul -Wno-incompatible-pointer-types
endif

ifneq ($(findstring $(TARGET_ARCH_ABI), armeabi armeabi-v7a),)
FG_SRCS += \
	inflate.c \
	simd_stub.c \

endif

ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
FG_SRCS += \
	adler32_simd.c \
	simd_stub.c \
	contrib/optimizations/inffast_chunky.c \
	contrib/optimizations/inflate.c \

FG_CFLAGS := -DADLER32_SIMD_NEON
FG_INCLUDES := contrib/optimizations/arm
endif

ifneq ($(findstring $(TARGET_ARCH_ABI), mips mips64),)
FG_SRCS += \
	inflate.c \
	simd_stub.c \

endif
