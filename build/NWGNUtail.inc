#
# This contains final targets and should be included at the end of any
# NWGNUmakefile file
#

#
# If we are going to create an nlm, make sure we have assigned variables to
# use during the link.
#
ifndef NLM_NAME
NLM_NAME = $(TARGET_nlm)
endif

ifndef NLM_DESCRIPTION
NLM_DESCRIPTION = $(NLM_NAME)
endif

ifndef NLM_THREAD_NAME
NLM_THREAD_NAME = $(NLM_NAME) Thread
endif

ifndef NLM_SCREEN_NAME
NLM_SCREEN_NAME = DEFAULT
endif

ifndef NLM_COPYRIGHT
NLM_COPYRIGHT = Licensed under the Apache License, Version 2.0
endif

ifeq "$(NLM_FLAGS)" ""
#NLM_FLAGS = AUTOUNLOAD, PSEUDOPREEMPTION
NLM_FLAGS = flag_on 72
endif

ifeq "$(NLM_STACK_SIZE)" ""
NLM_STACK_SIZE = 65536
endif

ifeq "$(NLM_ENTRY_SYM)" ""
NLM_ENTRY_SYM = _LibCPrelude
endif

ifeq "$(NLM_EXIT_SYM)" ""
NLM_EXIT_SYM = _LibCPostlude
endif

ifeq "$(NLM_VERSION)" ""
NLM_VERSION = $(VERSION)
endif

# if APACHE_UNIPROC is defined, don't include XDCData
ifndef APACHE_UNIPROC
ifneq "$(XDCDATA)" ""
NLM_XDCDATA = $(XDCDATA)
else
NLM_XDCDATA = $(APR)/misc/netware/apr.xdc
endif
endif
 
#
# Create dependency lists based on the files available
#

CCOPT_DEPENDS			= \
				$(APRBUILD)/NWGNUhead.inc \
				$(APRBUILD)/NWGNUenvironment.inc \
				$(APRBUILD)/NWGNUtail.inc \
				NWGNUmakefile \
				$(CUSTOM_INI) \
				$(EOLIST)

CPPOPT_DEPENDS			= \
				$(APRBUILD)/NWGNUhead.inc \
				$(APRBUILD)/NWGNUenvironment.inc \
				$(APRBUILD)/NWGNUtail.inc \
				NWGNUmakefile \
				$(CUSTOM_INI) \
				$(EOLIST)

$(NLM_NAME)_LINKOPT_DEPENDS	= \
				$(TARGET_lib) \
				$(APRBUILD)/NWGNUenvironment.inc \
				NWGNUmakefile \
				$(APRBUILD)/NWGNUtail.inc \
				$(CUSTOM_INI) \
				$(VERSION_INC) \
				$(EOLIST)

ifeq "$(words $(strip $(TARGET_lib)))" "1"
LIB_NAME			= $(basename $(notdir $(TARGET_lib)))
$(LIB_NAME)_LIBLST_DEPENDS	= \
				$(FILES_lib_objs) \
				$(APRBUILD)/NWGNUenvironment.inc \
				NWGNUmakefile \
				$(APRBUILD)/NWGNUtail.inc \
				$(CUSTOM_INI) \
				$(EOLIST)
endif

ifeq "$(wildcard NWGNU$(LIB_NAME))" "NWGNU$(LIB_NAME)"
$(LIB_NAME)_LIBLST_DEPENDS	+= NWGNU$(LIB_NAME)
endif

ifeq "$(wildcard NWGNU$(NLM_NAME))" "NWGNU$(NLM_NAME)"
$(NLM_NAME)_LINKOPT_DEPENDS	+= NWGNU$(NLM_NAME)
CCOPT_DEPENDS			+= NWGNU$(NLM_NAME)
CPPOPT_DEPENDS			+= NWGNU$(NLM_NAME)
endif

#
# Generic compiler rules
#

ifneq "$(MAKECMDGOALS)" "clean"
ifneq "$(findstring clobber_,$(MAKECMDGOALS))" "clobber_"
$(APRBUILD)/NWGNUversion.inc : $(APRBUILD)/nw_ver.awk $(APR)/include/apr_version.h
	@echo $(DL)GEN  $@$(DL)
	$(AWK) -f $^ $(APR)/.svn/all-wcprops > $@

-include $(APRBUILD)/NWGNUversion.inc

ifneq "$(strip $(VERSION_STR))" ""
VERSION_INC = $(APRBUILD)/NWGNUversion.inc
else
VERSION		= 2,0,0
VERSION_STR	= 2.0.0
VERSION_MAJMIN	= 20
endif
endif
endif
ifeq "$(USE_SVNREV)" "1"
ifneq "$(strip $(SVN_REVISION))" ""
CFLAGS	+= -DAPR_IS_DEV_STRING=\"$(SVN_REVISION)\"
endif
endif

ifeq "$(words $(strip $(TARGET_nlm)))" "1"
INCLUDE_BLDCMDS	= 1
CCOPT_NAME	= $(NLM_NAME)
endif

ifeq "$(words $(strip $(TARGET_lib)))" "1"
INCLUDE_BLDCMDS	= 1
CCOPT_NAME	= $(LIB_NAME)
endif

CCFLAGS = 
ifneq "$(strip $(CFLAGS))" ""
CCFLAGS += $(CFLAGS)
endif
ifneq "$(strip $(XCFLAGS))" ""
CCFLAGS += $(XCFLAGS)
endif
ifneq "$(strip $(XINCDIRS))" ""
CCFLAGS += $(foreach xincdir,$(strip $(XINCDIRS)),-I$(xincdir))
endif
ifneq "$(strip $(INCDIRS))" ""
CCFLAGS += $(foreach incdir,$(strip $(INCDIRS)),-I$(incdir))
endif
ifneq "$(strip $(DEFINES))" ""
CCFLAGS += $(DEFINES)
endif
ifneq "$(strip $(XDEFINES))" ""
CCFLAGS += $(XDEFINES)
endif

ifeq "$(INCLUDE_BLDCMDS)" "1"

$(OBJDIR)/%.o: %.c $(CCOPT_DEPENDS)
	@echo $(DL)CC   $<$(DL)
ifdef VERBOSE
	@echo CCOPT_DEPENDS=$(CCOPT_DEPENDS)
endif
	$(CC) $(CCFLAGS) -c -o $@ $<

CCPFLAGS = $(CPFLAGS) $(CCFLAGS)

$(OBJDIR)/%.o: %.cpp $(CPPOPT_DEPENDS)
	@echo $(DL)CPP  $<$(DL)
ifdef VERBOSE
	@echo CPPOPT_DEPENDS=$(CPPOPT_DEPENDS)
endif
	$(CPP) $(CCPFLAGS) -c -o $@ $<

endif # one target nlm or lib

#
# Rules to build libraries
#

# If we only have one target library then build it

ifeq "$(words $(strip $(TARGET_lib)))" "1"

$(TARGET_lib) : $(OBJDIR)/$(LIB_NAME)_lib.lst
	$(call DEL,$@)
	@echo $(DL)AR   $@$(DL)
	$(AR) $@ @$<
ifdef RANLIB
	$(RANLIB) $@
endif

$(OBJDIR)/aprlib_lib.lst: $(aprlib_LIBLST_DEPENDS)
	$(call DEL,$@)
ifneq "$(strip $(FILES_lib_objs))" ""
	@echo $(DL)GEN  $@$(DL)
	@echo $(DL)$(wordlist 1, 10, $(FILES_lib_objs))$(DL)>> $@
	@echo $(DL)$(wordlist 11, 20, $(FILES_lib_objs))$(DL)>> $@
	@echo $(DL)$(wordlist 21, 30, $(FILES_lib_objs))$(DL)>> $@
	@echo $(DL)$(wordlist 31, 40, $(FILES_lib_objs))$(DL)>> $@
	@echo $(DL)$(wordlist 41, 50, $(FILES_lib_objs))$(DL)>> $@
	@echo $(DL)$(wordlist 51, 60, $(FILES_lib_objs))$(DL)>> $@
	@echo $(DL)$(wordlist 61, 70, $(FILES_lib_objs))$(DL)>> $@
	@echo $(DL)$(wordlist 71, 80, $(FILES_lib_objs))$(DL)>> $@
	@echo $(DL)$(wordlist 81, 90, $(FILES_lib_objs))$(DL)>> $@
	@echo $(DL)$(wordlist 91, 100, $(FILES_lib_objs))$(DL)>> $@
	@echo $(DL)$(wordlist 101, 110, $(FILES_lib_objs))$(DL)>> $@
	@echo $(DL)$(wordlist 111, 120, $(FILES_lib_objs))$(DL)>> $@
endif

$(OBJDIR)/%_lib.lst: $($(LIB_NAME)_LIBLST_DEPENDS)
	$(call DEL,$@)
ifneq "$(strip $(FILES_lib_objs))" ""
	@echo $(DL)GEN  $@$(DL)
	@echo $(DL)$(FILES_lib_objs)$(DL)>> $@
endif

else # We must have more than one target library so load the individual makefiles

$(OBJDIR)/%.lib: NWGNU% $(APRBUILD)/NWGNUhead.inc $(APRBUILD)/NWGNUtail.inc $(APRBUILD)/NWGNUenvironment.inc FORCE
	@echo $(DL)Calling $<$(DL)
	$(MAKE) -f $< $(MAKECMDGOALS) RELEASE=$(RELEASE)

endif

#
# Rules to build nlms.
#

# If we only have one target NLM then build it
ifeq "$(words $(strip $(TARGET_nlm)))" "1"

$(TARGET_nlm) : $(FILES_nlm_objs) $(FILES_nlm_libs) $(OBJDIR)/$(NLM_NAME)_link.opt $(OBJDIR)/$(NLM_NAME)_link.def
	@echo $(DL)LINK $@$(DL)
	$(LINK) @$(OBJDIR)/$(NLM_NAME)_link.opt

# This will force the link option file to be rebuilt if we change the
# corresponding makefile

$(OBJDIR)/$(NLM_NAME)_link.opt : $($(NLM_NAME)_LINKOPT_DEPENDS)
	$(call DEL,$@)
	@echo $(DL)GEN  $@$(DL)
ifeq "$(findstring mwldnlm,$(LINK))" "mwldnlm" # for Metrowerks CodeWarrior
	@echo $(DL)# Do not edit this file - it is created by make!$(DL) > $@
	@echo $(DL)# All your changes will be lost!!$(DL)>> $@
	@echo $(DL)-nlmversion=$(NLM_VERSION)$(DL)>> $@
	@echo $(DL)-warnings off$(DL)>> $@
	@echo $(DL)-zerobss$(DL)>> $@
	@echo $(DL)-nodefaults$(DL)>> $@
	@echo $(DL)-map $(OBJDIR)/$(NLM_NAME).map$(DL)>> $@
	@echo $(DL)-o $(TARGET_nlm)$(DL)>> $@
ifneq "$(FILE_nlm_copyright)" ""
	@-$(CAT) $(FILE_nlm_copyright) >> $@
endif
ifeq "$(RELEASE)" "debug"
	@echo $(DL)-g$(DL)>> $@
	@echo $(DL)-sym codeview4$(DL)>> $@
	@echo $(DL)-osym $(OBJDIR)/$(NLM_NAME).sym$(DL)>> $@
endif
	@echo $(DL)-sym internal$(DL)>> $@
	@echo $(DL)-L$(APR)/misc/netware$(DL)>> $@
	@echo $(DL)-L"$(METROWERKS)/Novell Support/Metrowerks Support/Libraries/Runtime"$(DL)>> $@
	@echo $(DL)-L"$(METROWERKS)/Novell Support/Metrowerks Support/Libraries/MSL C++"$(DL)>> $@
ifneq "$(IPV6)" ""
	@echo $(DL)-L$(NOVELLLIBC)/include/winsock/IPV6$(DL)>> $@
endif
	@echo $(DL)-L$(NOVELLLIBC)/imports$(DL)>> $@
ifneq "$(strip $(XLFLAGS))" ""
	@echo $(DL)$(XLFLAGS)$(DL)>> $@
endif
ifneq "$(strip $(FILES_nlm_objs))" ""
	@echo $(DL)$(foreach objfile,$(strip $(FILES_nlm_objs)),$(objfile))$(DL)>> $@
endif
ifneq "$(FILES_nlm_libs)" ""
	@echo $(DL)$(foreach libpath,$(dir $(strip $(FILES_nlm_libs))),-L$(libpath))$(DL)>> $@
	@echo $(DL)$(foreach libfile,$(notdir $(strip $(FILES_nlm_libs))),-l$(libfile))$(DL)>> $@
endif
ifneq "$(FILES_nlm_Ximports)" ""
	@echo $(DL)$(foreach imppath,$(dir $(strip $(FILES_nlm_Ximports))),$(subst @,-L,$(imppath)))$(DL)>> $@
endif
	@echo $(DL)-commandfile $(@:.opt=.def)$(DL)>> $@
else # for GNU nlmconv
	@echo $(DL)-UT $(@:.opt=.def)$(DL)>> $@
endif

$(OBJDIR)/$(NLM_NAME)_link.def : $($(NLM_NAME)_LINKOPT_DEPENDS)
	$(call DEL,$@)
	@echo $(DL)GEN  $@$(DL)
	@echo $(DL)# Do not edit this file - it is created by make!$(DL)> $@
	@echo $(DL)# All your changes will be lost!!$(DL)>> $@
ifneq "$(FILE_nlm_msg)" ""
	@echo $(DL)Messages $(FILE_nlm_msg)$(DL)>> $@
endif
ifneq "$(FILE_nlm_hlp)" ""
	@echo $(DL)Help $(FILE_nlm_hlp)$(DL)>> $@
endif
ifeq "$(FILE_nlm_copyright)" ""
	@echo $(DL)copyright "$(NLM_COPYRIGHT)"$(DL)>> $@
endif
	@echo $(DL)description "$(NLM_DESCRIPTION)"$(DL)>> $@
	@echo $(DL)threadname "$(NLM_THREAD_NAME)"$(DL)>> $@
	@echo $(DL)screenname "$(NLM_SCREEN_NAME)"$(DL)>> $@
	@echo $(DL)stacksize $(subst K,000,$(subst k,K,$(strip $(NLM_STACK_SIZE))))$(DL)>> $@
	@echo $(DL)$(strip $(NLM_FLAGS))$(DL)>> $@
	@echo $(DL)start $(NLM_ENTRY_SYM)$(DL)>> $@
	@echo $(DL)exit $(NLM_EXIT_SYM)$(DL)>> $@
ifneq "$(NLM_CHECK_SYM)" ""
	@echo $(DL)check $(NLM_CHECK_SYM)$(DL)>> $@
endif
ifneq "$(FILES_nlm_modules)" ""
	@echo $(DL)module $(strip $(FILES_nlm_modules))$(DL)>> $@
endif
ifneq "$(FILES_nlm_imports)" ""
	@echo $(DL)import $(strip $(FILES_nlm_imports))$(DL)>> $@
endif
ifeq "$(findstring mwldnlm,$(LINK))" "mwldnlm" # for Metrowerks CodeWarrior
ifneq "$(FILES_nlm_Ximports)" ""
	@echo $(DL)import $(foreach import,$(notdir $(strip $(FILES_nlm_Ximports))),@$(import))$(DL)>> $@
endif
ifneq "$(FILES_nlm_exports)" ""
	@echo $(DL)export $(foreach export,$(subst $(SPACE),$(COMMA),$(strip $(FILES_nlm_exports))),$(export))$(DL)>> $@
endif
ifneq "$(NLM_XDCDATA)" ""
	@echo $(DL)xdcdata $(notdir $(NLM_XDCDATA))$(DL)>> $@
endif
	@echo $(DL)map $(OBJDIR)/$(NLM_NAME).map$(DL)>> $@
else # for GNU nlmconv
ifneq "$(FILES_nlm_Ximports)" ""
	@echo $(DL)import $(strip $(FILES_nlm_Ximports))$(DL)>> $@
endif
ifneq "$(FILES_nlm_exports)" ""
	@echo $(DL)export $(foreach export,$(subst $(SPACE),$(COMMA),$(strip $(FILES_nlm_exports))),$(export))$(DL)>> $@
endif
ifneq "$(NLM_XDCDATA)" ""
	@echo $(DL)xdcdata $(NLM_XDCDATA)$(DL)>> $@
endif
ifneq "$(strip $(FILES_nlm_objs))" ""
	@echo $(DL)input $(strip $(FILES_nlm_objs))$(DL)>> $@
endif
ifneq "$(FILES_nlm_libs)" ""
	@echo $(DL)input $(strip $(FILES_nlm_libs))$(DL)>> $@
endif
	@echo $(DL)output $(TARGET_nlm)$(DL)>> $@
ifeq "$(RELEASE)" "debug"
	@echo $(DL)debug$(DL)>> $@
endif
	@echo $(DL)version $(NLM_VERSION) $(DL)>> $@
endif

else # more than one target so look for individual makefiles.

# Only include these if NO_LICENSE_FILE isn't set to prevent excessive
# recursion

ifndef NO_LICENSE_FILE

$(OBJDIR)/%.nlm: NWGNU% $(APRBUILD)/NWGNUhead.inc $(APRBUILD)/NWGNUtail.inc $(APRBUILD)/NWGNUenvironment.inc $(CUSTOM_INI) $(VERSION_INC) FORCE
	@echo $(DL)Calling $<$(DL)
	$(MAKE) -f $< $(MAKECMDGOALS) RELEASE=$(RELEASE)
	@$(ECHONL)

else

$(TARGET_nlm):

endif # NO_LICENSE_FILE

endif # multiple targets

$(INSTDIRS) ::
	$(call MKDIR,$@)


