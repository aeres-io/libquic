# Copyright 2018 - Filegear - All Rights Reserved

SRCROOT = $(realpath ..)

DIRS  = \
	third_party \
	quic

.PHONY: all test clean $(DIRS)

all: $(DIRS)

test: $(DIRS)

clean: $(DIRS)

$(DIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS)
