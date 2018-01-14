# Copyright 2018 - Filegear - All Rights Reserved

ROOT = $(realpath ../../..)

include $(ROOT)/Config.mk

include Makefile.Common.mk

#ifeq ($(OSTYPE), android)
FG_SHAREDLIB := $(FG_TARGETNAME)
FG_CFLAGS += -DBORINGSSL_SHARED_LIBRARY
#else
#ifeq ($(OSTYPE), linux)
#FG_LIB := $(FG_TARGETNAME)
#endif
#endif
SYSLIBS := -pthread -shared

ifeq ($(OSTYPE), android)
include $(ROOT)/build/Makefile.Android.mk
else
ifeq ($(OSTYPE), linux)
include $(ROOT)/build/Makefile.Linux.mk
endif
endif
