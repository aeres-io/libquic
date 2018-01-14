# Copyright 2018 - Filegear - All Rights Reserved 

ifeq ($(ROOT),)
$(error "Please define ROOT")
endif

include $(ROOT)/Config.mk

VPATH += $(OUT_BIN)/ $(BINDIR)
CCFLAGS += -I$(OUT_INCLUDE)/out/include
CXXFLAGS += -std=c++14
CFLAGS += -std=c99

include $(BUILD_ROOT)/Cpp.mk
