# Copyright 2018 - Filegear - All Rights Reserved

SRCROOT = $(realpath ../..)

DIRS  = \
	boringssl \

ifeq ($(OSTYPE), android)
DIRS += zlib
endif

.PHONY: all test clean $(DIRS)

all: $(DIRS)

test: $(DIRS)

clean: $(DIRS)

$(DIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS)
