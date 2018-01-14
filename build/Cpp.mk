# Copyright 2018 - Filegear - All Rights Reserved

ifeq ($(ROOT),)
$(error "Please define ROOT")
endif

include $(ROOT)/Config.mk

PHEADERS_ = $(patsubst %,$(INCDIR)/%,$(PHEADERS))

ifneq ($(findstring $(PRODUCT_SKU),GEAR),)
CCFLAGS += -DPRODUCT_SKU=SKU_$(subst -,_,$(PRODUCT_SKU))
else
CCFLAGS += -DPRODUCT_SKU=SKU_GEAR
endif

ifeq ($(PROFILE), 1)
CCFLAGS += -pg
LDFLAGS += -lc -pg
endif

ifeq ($(THISBUILDTYPE),debug)
CCFLAGS += -O0 -DDEBUG
else
CCFLAGS += -O2 -DNDEBUG
endif

ifeq ($(BINDIR),)
BINDIR = bin
endif

COPY_ = $(patsubst %,$(BINDIR)/%,$(COPY))

LDFLAGS += -Wl,-rpath,.

# Compiler flags for cross compile 32-bit target on 64-bit OS
ifneq ($(findstring $(BUILDARCH),i386 i486 i586 i686 i786 i886 i986 x86 i86pc),)
ifeq ($(findstring $(BUILDHOST),i386 i486 i586 i686 i786 i886 i986 x86 i86pc),)
CROSS_CCFLAGS += -m32
endif
endif

ifneq ($(findstring $(BUILDARCH),i386 i486 i586 i686 i786 i886 i986 x86 i86pc),)
CCFLAGS += -DRT_ARCH_X86
else
ifneq ($(findstring $(BUILDARCH),x86_64 amd64),)
CCFLAGS += -DRT_ARCH_AMD64
else
ifeq ($(BUILDARCH), arm)
CCFLAGS += -DRT_ARCH_ARM
endif
endif
endif

ifeq ($(SDL_LIB), 1)
ifeq ($(BUILDARCH), arm)
CCFLAGS += -I$(SYSROOT)/usr/local/include/SDL -I$(SYSROOT)/usr/include/SDL
LDFLAGS += -lSDL
else
CCFLAGS += `sdl-config --cflags`
LDFLAGS += `sdl-config --libs`
endif
endif

ifeq ($(OSTYPE), darwin)
CCFLAGS += -DRT_OS_DARWIN -D__DARWIN__ -I/opt/local/include
endif

ifeq ($(VBOX_IPRT), 1)

# Enable VBox Runtime if VBOX_IPRT is set to 1
ifeq ($(BUILDARCH), i686)
BUILDARCH_VBOX = x86
else
ifeq ($(BUILDARCH), x86_64)
BUILDARCH_VBOX = amd64
else
ifeq ($(BUILDARCH), arm)
BUILDARCH_VBOX = arm
endif
endif
endif

LDFLAGS += -L$(VBOXLIB)
CCFLAGS += -DIN_RING3 -I$(ROOT_VBOX)/include

endif

ifeq ($(BUILDARCH), arm)
CCFLAGS += -DRT_ARCH_ARM_EABI -D__ARM_EABI__ -DRTLOG_REL_DISABLED -march=armv7-a -mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=vfpv3-d16 --sysroot=/ -Wno-poison-system-directories -I$(HOME)/package/usr/include -I$(HOME)/package/usr/local/include
LDFLAGS += -L/usr/lib -L/usr/local/lib -L$(HOME)/package/usr/lib -L$(HOME)/package/usr/local/lib --sysroot=/ -Wno-poison-system-directories
endif


ifneq ($(PROGRAM),)
TARGETEXE=$(BINDIR)/$(PROGRAM)
TARGETNAME ?= $(PROGRAM)
TARGETNAMESTR=$(PROGRAM)
TARGETSYM=$(SYMDIR)/$(PROGRAM).symbol
endif

ifneq ($(LIB),)
TARGETLIB=$(BINDIR)/$(LIB).a
TARGETNAME=$(LIB).a
CCFLAGS += -fPIC
TARGETNAMESTR=$(LIB)
endif

ifneq ($(SHARELIB),)
TARGETSO=$(BINDIR)/$(SHARELIB).$(SHARELIBEXT)
TARGETNAME=$(SHARELIB).$(SHARELIBEXT)
CCFLAGS += -fPIC -fvisibility=default
TARGETNAMESTR=$(SHARELIB)
TARGETSYM=$(SYMDIR)/$(SHARELIB).symbol
endif

TARGETPOT=$(TARGETNAMESTR).pot

OBJTGT = $(OBJDIR)/$(TARGETNAME)

_OBJS0   = ${SRCS:.cpp=.o}
_OBJS1   = ${_OBJS0:.cc=.o}
_OBJS2   = ${_OBJS1:.S=.o}
_OBJS    = ${_OBJS2:.c=.o}
OBJS    = $(patsubst %,$(OBJTGT)/%,$(_OBJS))

_DEPS0   = ${SRCS:.cpp=.dep}
_DEPS1   = ${SRCS:.cc=.dep}
_DEPS2   = ${SRCS:.S=.dep}
_DEPS    = ${SRCS:.c=.dep}
DEPS     = $(patsubst %,$(OBJTGT)/%,$(_DEPS0))
DEPS     += $(patsubst %,$(OBJTGT)/%,$(_DEPS1))
DEPS     += $(patsubst %,$(OBJTGT)/%,$(_DEPS2))
DEPS     += $(patsubst %,$(OBJTGT)/%,$(_DEPS))

CCFLAGS +=-g -pipe -Wall -Wno-missing-field-initializers -Wno-trigraphs -fdiagnostics-show-option -Wno-long-long -Wno-variadic-macros -fno-omit-frame-pointer -fno-strict-aliasing -D_GNU_SOURCE
ifdef WARNING_CLEAN
CCFLAGS += -Werror
endif

ifneq ($(LOCALIZE),)
POTTGT = $(OBJDIR)/$(TARGETPOT)
ifeq ($(LOCALIZATION_PREFIX),)
LOCALIZATION_PREFIX=_  
endif
endif

ifeq ($(DISABLE_PROTECT),)
ifneq ($(PROTECT),)
STRIP_SYMBOLS = 1
endif
endif

ifneq ($(CCFLAGS_OVERRIDE),)
	CCFLAGS = $(CCFLAGS_OVERRIDE)
endif

ifneq ($(LDFLAGS_OVERRIDE),)
	LDFLAGS = $(LDFLAGS_OVERRIDE)
endif

define cc-command
@echo "compile: $<"
@$(CC) $(CROSS_CCFLAGS) $(CCFLAGS) $(CXXFLAGS) -Wp,-MD,${patsubst %.o,%.dep,$@} -Wp,-MT,$@ -Wp,-MP -o $@ -c $<
endef

define c-command
@echo "compile: $<"
@$(CC) $(CROSS_CCFLAGS) $(CCFLAGS) $(CFLAGS) -Wp,-MD,${patsubst %.o,%.dep,$@} -Wp,-MT,$@ -Wp,-MP -o $@ -c $<
endef

all:: objdir $(TARGETEXE) $(TARGETLIB) $(TARGETSO) $(PHEADERS_) $(POTTGT) $(POSTBUILD) $(COPY_)

objdir:
ifeq ($(THISBUILDTYPE),debug)
	@echo !!!! this is a DEBUG build !!!!
endif
ifeq ($(BUILDTYPE),debug)
	@echo !!!! using DEBUG libs !!!!
endif
	-@mkdir -p $(OBJTGT)
	-@mkdir -p $(BINDIR)
	-@mkdir -p $(INCDIR)
	-@mkdir -p $(SYMDIR)

$(PHEADERS_): $(INCDIR)/%: %
	@echo "copy: $@"
	@mkdir -p $$(dirname $@)
	@cp $< $@

$(OBJTGT)/%.o: %.cpp
	@mkdir -p $$(dirname $@)
	$(cc-command)

$(OBJTGT)/%.o: %.cc
	@mkdir -p $$(dirname $@)
	$(cc-command)

$(OBJTGT)/%.o: %.c
	@mkdir -p $$(dirname $@)
	$(c-command)

$(OBJTGT)/%.o: %.S
	@mkdir -p $$(dirname $@)
	$(c-command)

$(COPY_): $(BINDIR)/%: %
	@echo "copy: $@"
	@cp -f $< $@

$(TARGETEXE): $(OBJS) $(LIBS)
	@echo "link: $@"
	@${CXX} ${CROSS_CCFLAGS} ${LDFLAGS} -o $@ $^ $(SYSLIBS)
ifneq ($(STRIP_SYMBOLS),)
	@objcopy --only-keep-debug $(TARGETEXE) $(TARGETSYM)
	@strip -s $(TARGETEXE)
endif

$(TARGETLIB): $(OBJS)
	@echo "link: $@"
	@rm -f $@
	@ar -cvq $@ $^

$(TARGETSO): $(OBJS) $(LIBS)
	@echo "link: $@"
	@$(CXX) ${CROSS_CCFLAGS} ${LDFLAGS} -o $@ $^ $(SYSLIBS)
ifneq ($(STRIP_SYMBOLS),)
	@objcopy --only-keep-debug $(TARGETSO) $(TARGETSYM)
	@strip -s $(TARGETSO)
endif

$(POTTGT): $(SRCS)
	@echo "xgettext: $@"
	@xgettext -k$(LOCALIZATION_PREFIX) -o $@ --package-name=$(TARGETNAMESTR) --msgid-bugs-address=support@filegear.com --copyright-holder=FileGear $^

clean:: $(POSTCLEAN)
	-@rm -f $(TARGETEXE)
	-@rm -f $(TARGETLIB)
	-@rm -f $(TARGETSO)
	-@rm -Rf $(OBJTGT)
	-@rm -f $(POTTGT)
	-@for d in $(COPY_); do ( rm -f $$d ); done

-include $(DEPS)
