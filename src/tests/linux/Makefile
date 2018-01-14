# Copyright 2018 - Filegear - All Rights Reserved 

ROOT=$(realpath ../../..)

include $(ROOT)/Config.mk

PROGRAM := test-quic

SRCS := \
	command_line_options.cc \
	frtc.cpp \
	mongoose.c \
	quic_gear_session_delegate.cc \
	quic_relay_client_config.cc


CCFLAGS = -I. -I../../quic -I../../quic/gen -I../.. -I../../third_party/boringssl/src/include -I../../quic/third_party/protobuf/src -DSSL_LIB=\"libboringssl.so\"

SYSLIBS := -L$(BINDIR) -lquic -lboringssl

SYSLIBS += -pthread -ldl -lz

WARNING_CLEAN = 1

include $(ROOT)/Cpp.mk
