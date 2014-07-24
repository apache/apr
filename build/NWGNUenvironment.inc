#
# Setup needed Tools and Libraries
#

ifeq "$(wildcard $(APR_WORK)/build/NWGNUcustom.inc)" "$(APR_WORK)/build/NWGNUcustom.inc"
include $(APR_WORK)/build/NWGNUcustom.inc
CUSTOM_INI = $(AP_WORK)/NWGNUcustom.ini
endif

ifndef VERBOSE
.SILENT:
endif

#
# Treat like an include
#
ifndef EnvironmentDefined

#
# simple macros for parsing makefiles
#
EOLIST:=
EMPTY :=
COMMA := ,
SPACE := $(EMPTY) $(EMPTY)

#
# Base environment
#

# Try and handle case issues
ifndef NOVELLLIBC
ifdef NovellLibC
NOVELLLIBC = $(NovellLibC)
endif
endif

ifndef NOVELLLIBC
NOVELLLIBC = C:/novell/ndk/libc
endif
ifneq "$(wildcard $(NOVELLLIBC)/include/ndkvers.h)" "$(NOVELLLIBC)/include/ndkvers.h"
$(error NOVELLLIBC does not point to a valid Novell LIBC SDK) 
endif

ifdef EXPATSDK
ifeq "$(wildcard $(EXPATSDK)/include/expat.h)" "$(EXPATSDK)/include/expat.h"
EXPAT_IMP = $(EXPATSDK)/imports/expatlbc.imp
EXPAT_INC = $(EXPATSDK)/include
EXPAT_LIB = $(EXPATSDK)/lib/expat.lib
EXPAT_NLM = EXPATLBC
else
$(error EXPATSDK does not point to a valid EXPAT SDK) 
endif
endif
ifdef EXPATSRC
ifeq "$(wildcard $(EXPATSRC)/lib/xmlparse.c)" "$(EXPATSRC)/lib/xmlparse.c"
EXPAT_INC = $(EXPATSRC)/lib
EXPAT_LIB = $(EXPATSRC)/lib/$(OBJDIR)/expat.lib
else
$(error EXPATSRC does not point to a valid EXPAT source tree) 
endif
endif
ifndef EXPAT_INC
$(error neither EXPATSDK nor EXPATSRC defined - cant compile without EXPAT SDK or source tree) 
endif

# If LM_LICENSE_FILE isn't defined, define a variable that can be used to
# restart make with it defined
ifndef LM_LICENSE_FILE
NO_LICENSE_FILE = NO_LICENSE_FILE
endif

#
# Set the Release type that you want to build, possible values are:
#
#  debug	- full debug switches are set
#  noopt	- normal switches are set
#  release	- optimization switches are set (default)

ifdef reltype
RELEASE = $(reltype)
endif

ifdef RELTYPE
RELEASE = $(RELTYPE)
endif

ifdef debug
RELEASE = debug
endif

ifdef DEBUG
RELEASE = debug
endif

ifdef noopt
RELEASE = noopt
endif

ifdef NOOPT
RELEASE = noopt
endif

ifdef optimized
RELEASE = release
endif

ifdef OPTIMIZED
RELEASE = release
endif

ifndef RELEASE
RELEASE = release
endif

ifeq "$(strip $(RELEASE))" "optimized"
RELEASE = release
endif

OBJDIR = obj_$(RELEASE)

#
# Setup compiler information
#

ifdef METROWERKS
# MetroWerks NLM tools
#ifndef METROWERKS
#METROWERKS = $(ProgramFiles)\Metrowerks\CodeWarrior
#endif

CC	= mwccnlm -w nocmdline -gccinc
CPP	= $(CC)
CPRE	= $(CC) -EP
LINK	= mwldnlm -w nocmdline
AR	= $(LINK) -type library -o
WIN_CC	= mwcc

ifneq ($(findstring /sh,$(SHELL)),/sh)
PATH:=$(PATH);$(METROWERKS)\bin;$(METROWERKS)\Other Metrowerks Tools\Command Line Tools
endif

# MetroWerks static Libraries
CLIB3S	= $(METROWERKS)/Novell Support/Metrowerks Support/Libraries/Runtime/mwcrtl.lib
MATH3S	=
PLIB3S	= $(METROWERKS)/Novell Support/Metrowerks Support/Libraries/MSL C++/MWCPP.lib

ifeq "$(OS)" "Windows_NT"
# MetroWerks Win32 build flags to create build tools
MWCW_MSL         = "$(METROWERKS)/MSL"
MWCW_W32         = "$(METROWERKS)/Win32-x86 Support"
CC_FOR_BUILD     = $(WIN_CC)
CFLAGS_FOR_BUILD =  -O2 -gccinc -nodefaults -proc 586 -w off
CFLAGS_FOR_BUILD += -ir $(MWCW_MSL) -ir $(MWCW_W32) -lr $(MWCW_MSL) -lr $(MWCW_W32)
CFLAGS_FOR_BUILD += -lMSL_All_x86.lib -lkernel32.lib -luser32.lib
else
# GNUC build flags to create build tools
CC_FOR_BUILD     = gcc
CFLAGS_FOR_BUILD = -Wall -O2
endif

# Base compile flags
# and prefix or precompiled header added here.

# The default flags are as follows:
#
# -w nocmdline          disable command-line driver/parser warnings
# -gccinc               search directory of referencing file first for #includes
# -Cpp_exceptions off   disable C++ exceptions
# -RTTI off             disable C++ run-time typing information
# -align 4              align on 4 byte bounderies
# -proc PII             generate code base on Pentium II instruction set
# -inst mmx             use MMX extensions (not used)

CFLAGS	= -proc PII -align 4
CPFLAGS	= -Cpp_exceptions off -RTTI off

ifdef CC_MAX_ERRORS
CFLAGS += -maxerrors $(CC_MAX_ERRORS)
else
CFLAGS += -maxerrors 1
endif

ifeq "$(REQUIRE_PROTOTYPES)" "1"
CFLAGS	+= -r
endif

# -g                    generate debugging information
# -O0                   level 0 optimizations
ifeq "$(RELEASE)" "debug"
CFLAGS	+= -g -O0
endif

# -O4,p                 level 4 optimizations, optimize for speed
ifeq "$(RELEASE)" "release"
CFLAGS	+= -O4,p
endif

PRELUDE	= $(NOVI)/libcpre.o

else
# GNU NLM tools (gcc / nlmconv)

CC	= gcc
CPP	= g++
CPRE	= $(CC) -P -E
LINK	= nlmconv
AR	= ar cru
RANLIB	= ranlib

CFLAGS	= -m32 -fno-builtin -fpcc-struct-return -fno-strict-aliasing
CFLAGS	+= -fpack-struct=4
CFLAGS	+= -Wall
CFLAGS	+= -Wdeclaration-after-statement -Wmissing-declarations -Wmissing-prototypes

ifdef MAINTAINER
CFLAGS	+= \
	-W -Wpointer-arith -Wwrite-strings -Wunused -Wshadow -Winline \
	-Wnested-externs -Wcast-align -Wtype-limits -Wstrict-prototypes \
	-Wfloat-equal -Wno-multichar -Wsign-compare -Wundef -Wendif-labels \
	-Wold-style-declaration -Wmissing-parameter-type -Wempty-body \
	-Wclobbered -Wignored-qualifiers -Wconversion -Wvla -pedantic
endif

# -g                    generate debugging information
# -O0                   level 0 optimizations
ifeq "$(RELEASE)" "debug"
CFLAGS	+= -g -O0
endif

# -O3                   level 3 optimizations, optimize for speed
ifeq "$(RELEASE)" "release"
CFLAGS	+= -O3
endif

PRELUDE	= $(NOVI)/libcpre.gcc.o

endif

# -include apr_arch_pre_nw.h      #include apr_arch_pre_nw.h for all files
CFLAGS	+= -include apr_arch_pre_nw.h


# Setup build tools
AWK	= awk

#
# Declare Command and tool macros here
#

ifeq ($(findstring /sh,$(SHELL)),/sh)
DEL	= rm -f $1
RMDIR	= rm -rf $1
MKDIR	= mkdir -p $1
COPY	= cp -av $1 $2
COPYR	= cp -ar $1 $2
ECHONL	= echo ""
DL	= '
CAT	= cat
else
ifeq "$(OS)" "Windows_NT"
DEL	= $(shell if exist $(subst /,\,$1) del /q /f 2>NUL $(subst /,\,$1))
RMDIR	= $(shell if exist $(subst /,\,$1)\NUL rd /q /s 2>NUL $(subst /,\,$1))
else
DEL	= $(shell if exist $(subst /,\,$1) del 2>NUL $(subst /,\,$1))
RMDIR	= $(shell if exist $(subst /,\,$1)\NUL deltree /y 2>NUL $(subst /,\,$1))
endif
ECHONL	= $(ComSpec) /c echo.
MKDIR	= $(shell if not exist $(subst /,\,$1)\NUL md 2>NUL $(subst /,\,$1))
COPY	= copy /y 2>NUL $(subst /,\,$1) $(subst /,\,$2)
COPYR	= xcopy /y /e 2>NUL $(subst /,\,$1) $(subst /,\,$2)
CAT	= type
endif

ifdef IPV6
ifndef USE_STDSOCKETS
USE_STDSOCKETS=1
endif
endif

NOVI	= $(NOVELLLIBC)/imports
INCDIRS	= $(NOVELLLIBC)/include

DEFINES	= -DNETWARE
ifdef USE_STDSOCKETS
DEFINES	+= -DUSE_BSD_SOCKETS
else
DEFINES	+= -DUSE_WINSOCK
INCDIRS	+= $(NOVELLLIBC)/include/winsock
endif
ifndef DEBUG
DEFINES	+= -DNDEBUG
endif

ifdef USE_STDSOCKETS
VERSION_SKT = (BSDSOCK)
else
VERSION_SKT = (WINSOCK)
endif

#
# Declare major project deliverables output directories here
#

ifdef DEST
INSTALL = $(DEST)
ifeq (\, $(findstring \,$(INSTALL)))
INSTDIRS = $(DEST)
endif
endif

ifdef dest
INSTALL = $(dest)
ifeq (\, $(findstring \,$(INSTALL)))
INSTDIRS = $(dest)
endif
endif

ifndef INSTALL
INSTALL = $(APR_WORK)/Dist
INSTDIRS = $(APR_WORK)/Dist
BASEDIR = Apr
endif

# Add support for building IPV6 alongside
ifneq "$(IPV6)" ""
DEFINES += -DNW_BUILD_IPV6
# INCDIRS := $(NOVELLLIBC)/include/winsock/IPV6 $(INCDIRS)

ifneq "$(findstring IPV6,$(OBJDIR))" "IPV6"
OBJDIR := $(OBJDIR)_IPV6
endif
        
ifneq "$(findstring IPV6,$(INSTALL))" "IPV6"
INSTALL := $(INSTALL)_IPV6
endif        

ifneq "$(findstring IPV6,$(INSTDIRS))" "IPV6"
INSTDIRS := $(INSTDIRS)_IPV6
endif

endif

ifdef DEST

ifndef BASEDIR
BASEDIR = Apache2
endif

endif

INSTALLBASE := $(INSTALL)/$(BASEDIR)

INSTDEVDIRS := \
	$(INSTDIRS) \
	$(INSTALLBASE) \
	$(INSTALLBASE)/include \
	$(INSTALLBASE)/lib \
	$(INSTALLBASE)/bin 

INSTDIRS += \
	$(INSTALLBASE)

#
# Common directories
#

APR		= $(subst \,/,$(APR_WORK))
APRBUILD	= $(APR)/build
APRXML		= $(APR)/xml
APRTEST		= $(APR)/test

#
# Internal Libraries
#

APRLIB		= $(APR)/$(OBJDIR)/aprlib.lib
APRXMLLIB	= $(APRXML)/$(OBJDIR)/xml.lib

#
# Additional general defines
#

EnvironmentDefined = 1
endif # ifndef EnvironmentDefined

# This is always set so that it will show up in lower directories

ifdef Path
Path = $(PATH)
endif

