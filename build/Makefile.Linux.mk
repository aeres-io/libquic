# Copyright 2018 - Filegear - All Rights Reserved

LIB := $(FG_LIB)
SHARELIB := $(FG_SHAREDLIB)
PROGRAM := $(FG_PROGRAM)

SRCS := $(FG_SRCS)

SRCS += $(FG_SRCS_LINUX)

CCFLAGS += $(FG_CFLAGS)

CXXFLAGS += $(FG_CPPFLAGS)

LIBS += $(patsubst %,%.a,$(FG_LIBS))

#LIBS += $(patsubst %,%.so,$(FG_SHAREDLIBS))

_INCLUDES = $(patsubst %,-I%,$(FG_INCLUDES))

CCFLAGS += $(_INCLUDES)

WARNING_CLEAN = 1

include $(ROOT)/Cpp.mk
