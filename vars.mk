PREFIX=/run/media/michael/FAST_ARCHIVE/Programming/bbs/maximus
SRC=/run/media/michael/FAST_ARCHIVE/Programming/bbs/vendor/maximus/unix/maximus
COMP=unix
PLATFORM=linux
BUILD=DEVEL

# Build platform: Linux (hand-configured for Castle; mirrors ./configure output)
# Configured 2026-09-25 for modern GCC without interactive configure

CC		= /usr/bin/gcc
CXX		= /usr/bin/g++
AR		= /usr/bin/ar
RANLIB		= /usr/bin/ranlib
YACC		= /usr/bin/bison -y
SED		= /usr/bin/sed
RM		= /usr/bin/rm -f
MV		= /usr/bin/mv -f
BSD_INSTALL	= /usr/bin/install
UNAME		= /usr/bin/uname
MAKEDEPEND	=

LDFLAGS		= -L.
OBJ_EXT		:= o
LIB_EXT		:= so
LIB_PREFIX	:= lib

-include	$(SRC)/vars_local.mk

BIN		= $(PREFIX)/bin
ETC		= $(PREFIX)/etc
LIBEXEC		= $(PREFIX)/libexec
LIB		= $(PREFIX)/lib
STATIC_LIB	= $(SRC)/lib

H_DIRS		:= unix/include slib msgapi btree max prot mex
SO_DIRS		:= unix slib msgapi btree prot mex comdll
LDFLAGS		+= $(foreach DIR, $(SO_DIRS), -L$(SRC)/$(DIR))
INCLUDES	:= -I. $(foreach DIR, $(H_DIRS), -I$(SRC)/$(DIR))
# modern gcc: allow multiple common symbols, PIC for .so, no -Werror
# Keep C at gnu89; C++ at gnu++98 so "nullptr" remains a valid identifier (pre-C++11).
CFLAGS		+= -O0 -g -fcommon -fPIC -Wno-error -std=gnu89 -funsigned-bitfields
CXXFLAGS	= -O0 -g -fcommon -fPIC -Wno-error -std=gnu++98 -fpermissive -funsigned-bitfields
LOADLIBES	= $(EXTRA_LOADLIBES) -lmax -lcompat -lcurses

include		$(SRC)/vars_$(PLATFORM).mk

LOADLIBES	+= $(OS_LIBS)
CPPFLAGS	= -DUNIX $(EXTRA_CPPFLAGS) $(INCLUDES) $(EXTRA_INCLUDES)
MDFLAGS		= -D__MAKEDEPEND__

ifneq (,$(findstring gcc,$(CC)))
include 	$(SRC)/vars_gcc.mk
endif

# Prefer -Wl,-rpath over ancient -Xlinker -R
LDFLAGS		:= $(filter-out -Xlinker,$(LDFLAGS))
LDFLAGS		:= $(filter-out -R%,$(LDFLAGS))
LDFLAGS		+= -Wl,-rpath,$(LIB)

.PHONY:		top build_debug depend

top:		all

build_debug:
		@echo
		@echo "BUILD        = $(BUILD)"
		@echo "PREFIX       = $(PREFIX)"
		@echo "SRC          = $(SRC)"
		@echo "COMP         = $(COMP)"
		@echo "PLATFORM     = $(PLATFORM)"
		@echo "CC           = $(CC)"
		@echo "CXX          = $(CXX)"
		@echo "LD           = $(LD)"
		@echo "YACC         = $(YACC)"
		@echo "MAKEDEPEND   = $(MAKEDEPEND)"
		@echo
		@echo "CFLAGS       = $(CFLAGS)"
		@echo "CXXFLAGS     = $(CXXFLAGS)"
		@echo "CPPFLAGS     = $(CPPFLAGS)"
		@echo "LDFLAGS      = $(LDFLAGS)"
		@echo "LOADLIBES    = $(LOADLIBES)"
		@echo "YFLAGS       = $(YFLAGS)"
		@echo "PATH         = $(PATH)"
		@echo "DEPEND_FILES = $(DEPEND_FILES)"
		@echo

# Implicit rules for constructing *.c, *.h from yacc input
# under either bison or yacc. Assumes !yacc == bison.

%.h: %.y
ifneq (,$(findstring yacc,$(YACC)))
	$(YACC) -d $<
	$(MV) y.tab.h $@
	$(RM) y.tab.c
else
	$(YACC) -d $< -o $*.c
endif

%.c: %.y
ifneq (,$(findstring yacc,$(YACC)))
	$(YACC) $<
	$(MV) y.tab.c $@
else
	$(YACC) $< -o $@
endif

ifneq ($(strip $(BUILD)),RELEASE) 
ifneq ($(strip $(NO_DEPEND_RULE)),TRUE)
-include depend.mk 
depend depend.mk: $(SRC)/vars.mk
	@echo "Building dependencies.."
	@-touch depend.mk 
ifneq ($(MAKEDEPEND),) 
	@-[ "$(DEPEND_FILES)" ] && $(MAKEDEPEND) $(MDFLAGS) -I. $(CPPFLAGS) -f depend.mk $(DEPEND_FILES)
endif 
endif
endif
