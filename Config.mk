# Copyright 2016 FileGear - All Rights Reserved

-include $(ROOT)/Local.mk

DEVROOT    ?= $(realpath $(ROOT))
BUILD_ROOT ?= $(DEVROOT)/build

include $(BUILD_ROOT)/Config.mk

SRC_ROOT = $(ROOT)/src

OUT_ROOT = $(ROOT)/out
OUT_INCLUDE = $(OUT_ROOT)/include
OUT_BLD = $(OUT_ROOT)/$(BLD)
OUT_BIN = $(OUT_BLD)/bin
OUT_LIB = $(OUT_BLD)/lib

ifeq ($(OSTYPE), linux)
TARGET_ARCH_ABI := $(BUILDARCH)
endif

