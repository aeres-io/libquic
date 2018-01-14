# Copyright 2018 - Filegear - All Rights Reserved

ROOT = $(realpath ../..)

include $(ROOT)/Config.mk

include Makefile.Common.mk

ifeq ($(OSTYPE), linux)
CCFLAGS += -I. -Ibase/third_party/libevent/linux -Inet/quic/core/proto `nss-config --cflags` `nspr-config --cflags` -Wno-error=unused-but-set-variable -Wno-error=unused-variable -Wno-error=sign-compare -Wno-error=attributes -Wno-error=switch -Wno-error=unused-function -Wno-error=unused-local-typedefs -DUSE_NSS_CERTS -DOS_LINUX
CXXFLAGS += -fno-exceptions
LDFLAGS += -Wl,-z,noexecstack -Wl,--as-needed -Wl,-z,origin

SYSLIBS += -L$(BINDIR) -lboringssl
SYSLIBS += -pthread -shared `nss-config --libs` `nspr-config --libs`
endif

ifeq ($(OSTYPE), android)
FG_INCLUDES += ../third_party/zlib
FG_LIBS += quic_zlib
endif

ifeq ($(OSTYPE), android)
include $(ROOT)/build/Makefile.Android.mk
else
ifeq ($(OSTYPE), linux)
include $(ROOT)/build/Makefile.Linux.mk
endif
endif
