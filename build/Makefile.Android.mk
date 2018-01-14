# Copyright 2018 - Filegear - All Rights Reserved

OUT_ANDROID_LIBS := $(OUT_ROOT)/android/libs
OUT_ANDROID_OBJS := $(OUT_ROOT)/android/obj
OUT_ANDROID_BINS := $(OUT_ROOT)/android/bin

ANDROID_ABI ?= armeabi-v7a arm64-v8a x86 x86_64
ANDROID_PLATFORM ?= 15
ANDROID_STL := c++_shared

ifneq ($(FG_SHAREDLIB),)
TARGET_NAME := $(FG_SHAREDLIB)
TARGET_FILENAME := lib$(FG_SHAREDLIB).$(SHARELIBEXT)
TARGET_OUT_PATH := $(OUT_ANDROID_LIBS)
else
ifneq ($(FG_LIB),)
TARGET_NAME := $(FG_LIB)
TARGET_FILENAME := lib$(FG_LIB).a
TARGET_OUT_PATH := $(OUT_ANDROID_OBJS)/local
else
TARGET_NAME := $(FG_PROGRAM)
TARGET_FILENAME := $(FG_PROGRAM)
TARGET_OUT_PATH := $(OUT_ANDROID_LIBS)
endif
endif

ifneq ($(VERBOSE),)
VERBOSE_FLAG := 1
else
VERBOSE_FLAG := 0
endif

.PHONY: all clean

all: Application.mk
	@export NDK_PROJECT_PATH=. && ndk-build NDK_APPLICATION_MK=./Application.mk NDK_LIBS_OUT=$(OUT_ANDROID_LIBS) NDK_OUT=$(OUT_ANDROID_OBJS) V=$(VERBOSE_FLAG)
	@for arch in $(ANDROID_ABI); do (echo "Binplace $$arch/$(TARGET_FILENAME)"; mkdir -p $(OUT_ANDROID_BINS)/$$arch; \cp -rf $(TARGET_OUT_PATH)/$$arch/$(TARGET_FILENAME) $(OUT_ANDROID_BINS)/$$arch/ ); done
	

Application.mk: $(FG_SRCS)
	@echo "APP_OPTIM := $(BUILDTYPE)" > Application.mk
	@echo "APP_ABI := $(ANDROID_ABI)" >> Application.mk
	@echo "APP_PLATFORM := android-$(ANDROID_PLATFORM)" >> Application.mk
	@echo "APP_STL := $(ANDROID_STL)" >> Application.mk
	@echo "NDK_TOOLCHAIN_VERSION := clang" >> Application.mk
	@echo "APP_BUILD_SCRIPT := Android.mk" >> Application.mk

clean:
	-@for arch in $(ANDROID_ABI); do rm -rf $(OUT_ANDROID_BINS)/$$arch/$(TARGET_FILENAME); done
	-@export NDK_PROJECT_PATH=. && ndk-build clean NDK_APPLICATION_MK=./Application.mk NDK_LIBS_OUT=$(OUT_ANDROID_LIBS) NDK_OUT=$(OUT_ANDROID_OBJS)
	-@rm -rf Application.mk