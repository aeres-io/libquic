# Copyright 2018 - Filegear - All Rights Reserved

ROOT = $(realpath ../../..)

include $(ROOT)/Config.mk

include Makefile.Common.mk

ifeq ($(OSTYPE), linux)
CCFLAGS += -Wno-error=parentheses
endif

ifeq ($(OSTYPE), android)
include $(ROOT)/build/Makefile.Android.mk
else
ifeq ($(OSTYPE), linux)
include $(ROOT)/build/Makefile.Linux.mk
endif
endif
