# Copyright 2018 FileGear - All Rights Reserved

###############################################################################
# Prerequisites
###############################################################################

BUILD_ROOT := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

-include $(BUILD_ROOT)/Local.mk

ifeq ($(ROOT),)
$(error "Please define ROOT variable to point to the repository's root folder")
endif

###############################################################################
# Depot Roots
###############################################################################

DEVROOT         ?= $(realpath $(BUILD_ROOT)/..)

###############################################################################
# Versioning
###############################################################################

BUILD_NUMBER := $(shell cat $(BUILD_ROOT)/BuildNumber)

PRODUCT_VERSION_MAJOR        ?= $(shell cat $(BUILD_ROOT)/BuildMajorVersion)
PRODUCT_VERSION_MINOR        ?= $(shell cat $(BUILD_ROOT)/BuildMinorVersion)
PRODUCT_VERSION_BUILD        ?= $(BUILD_NUMBER)
ifeq ($(shell uname -r | grep -o el7), el7)
	PRODUCT_VERSION_PLATFORM     ?= el7
endif
ifeq ($(shell uname -r | grep -o fc14), fc14)
	PRODUCT_VERSION_PLATFORM     ?= fc14
endif
ifeq ($(shell uname -r | grep -o fc17), fc17)
	PRODUCT_VERSION_PLATFORM     ?= fc17
endif
ifeq ($(shell uname -r | grep -o fc21), fc21)
	PRODUCT_VERSION_PLATFORM     ?= fc21
endif
ifeq ($(shell uname -r | grep -o fc23), fc23)
	PRODUCT_VERSION_PLATFORM     ?= fc23
endif
PRODUCT_VERSION_MILESTONE    ?= $(shell cat $(BUILD_ROOT)/BuildMilestone)
PRODUCT_VERSION_BRANCH       ?= $(shell git rev-parse --abbrev-ref HEAD)
PRODUCT_VERSION_USER         ?= $(shell whoami)
PRODUCT_VERSION_CODE         ?= $(PRODUCT_VERSION_USER)_$(PRODUCT_VERSION_BRANCH)_$(PRODUCT_VERSION_MILESTONE)
PRODUCT_VERSION               = $(PRODUCT_VERSION_MAJOR).$(PRODUCT_VERSION_MINOR)
PRODUCT_VERSION_FULL          = $(PRODUCT_VERSION).$(PRODUCT_VERSION_BUILD).$(PRODUCT_VERSION_PLATFORM)

###############################################################################
# Product   
###############################################################################

PRODUCT_SKU								?= MESH

ifneq (,$(filter $(PRODUCT_SKU),MESH))
PRODUCT_NAME=MESH
PRODUCT_DESC=Filegear Mesh
PRODUCT_EDITION=Standard
PACKAGE_NAME=mesh
endif

OEM_PRODUCT_NAME ?= $(PRODUCT_NAME)
OEM_PRODUCT_DESC ?= $(PRODUCT_DESC)

###############################################################################
# Platform & Build
###############################################################################

BUILDTYPE     ?= release
BUILDARCH     ?= $(shell uname -m)
BUILDHOST     ?= $(shell uname -m)
THISBUILDTYPE ?= $(BUILDTYPE)

ifeq ($(shell uname -r | cut -d '.' -f 5),fc14)
BUILD_PROFILE_SERVER ?= 1
BUILD_PROFILE_CLIENT ?= 1 
else
ifeq ($(shell uname -r | grep -o el7), el7)
BUILD_PROFILE_SERVER ?= 1
BUILD_PROFILE_CLIENT ?= 1
else
BUILD_PROFILE_SERVER ?= 0
BUILD_PROFILE_CLIENT ?= 1 
endif
endif

ifeq ($(OSTYPE),)
THISOSTYPE := $(shell uname)
ifeq ($(THISOSTYPE),Linux)
OSTYPE = linux
else
ifeq ($(THISOSTYPE),Darwin)
OSTYPE = darwin
endif
endif
endif

ifneq ($(findstring $(BUILDARCH),i386 i486 i586 i686 i786 i886 i986 x86 i86pc),)
VBOXBUILDARCH = x86
else
ifneq ($(findstring $(BUILDARCH),x86_64 amd64),)
VBOXBUILDARCH = amd64
else
ifeq ($(BUILDARCH), arm)
VBOXBUILDARCH = arm
else
$(error Unsupported build architecture: $(BUILDARCH))
endif
endif
endif

BLD    = $(OSTYPE).$(BUILDARCH)/$(BUILDTYPE)
THISBLD= $(OSTYPE).$(BUILDARCH)/$(THISBUILDTYPE)
OUTDIR = $(ROOT)/out
INCDIR = $(OUTDIR)/include
BLDDIR = $(OUTDIR)/$(THISBLD)
BINDIR = $(BLDDIR)/bin
OBJDIR = $(BLDDIR)/obj
LIBDIR = $(BLDDIR)/lib
SYMDIR = $(BLDDIR)/sym

DEPLOYROOT ?= $(HOME)/builds
DEPLOYDIR ?= $(DEPLOYROOT)/$(PRODUCT_VERSION_MAJOR)-$(PRODUCT_VERSION_MINOR)-$(PRODUCT_VERSION_BUILD).$(PRODUCT_VERSION_CODE)

ifeq ($(OSTYPE),darwin)
SHARELIBEXT=dylib
else
SHARELIBEXT=so
endif
