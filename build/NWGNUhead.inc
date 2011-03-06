#
# Obtain the global build environment
#

include $(APR_WORK)/build/NWGNUenvironment.inc

#
# Define base targets and rules
# 

TARGETS = libs nlms install clobber_libs clobber_nlms clean installdev

.PHONY : $(TARGETS) default all help $(NO_LICENSE_FILE)

# Here is where we will use the NO_LICENSE_FILE variable to see if we need to
# restart the make with it defined

ifdef NO_LICENSE_FILE

default: NO_LICENSE_FILE

all: NO_LICENSE_FILE

install :: NO_LICENSE_FILE

installdev :: NO_LICENSE_FILE

NO_LICENSE_FILE :
	$(MAKE) $(MAKECMDGOALS) -f NWGNUmakefile RELEASE=$(RELEASE) DEST="$(INSTALL)" LM_LICENSE_FILE="$(METROWERKS)/license.dat"

else # LM_LICENSE_FILE must be defined so use the real targets

default: $(SUBDIRS) libs nlms

all: $(SUBDIRS) libs nlms install

$(TARGETS) :: $(SUBDIRS)

endif #NO_LICENSE_FILE check

help :
	@echo targets for RELEASE=$(RELEASE):
	@echo (default) . . . . libs nlms
	@echo all . . . . . . . does everything (libs nlms install)
	@echo libs. . . . . . . builds all libs
	@echo nlms. . . . . . . builds all nlms
	@echo install . . . . . builds libs and nlms and copies install files to
	@echo                   "$(INSTALL)"
	@echo installdev. . . . copies headers and files needed for development to
	@echo                   "$(INSTALL)"
	@echo clean . . . . . . deletes $(OBJDIR) dirs, *.err, and *.map
	@echo clobber_all . . . deletes all possible output from the make
	@echo clobber_install . deletes all files in $(INSTALL)
	@$(ECHONL)
	@echo Multiple targets can be used on a single nmake command line -
	@echo (i.e. $(MAKE) clean all)
	@$(ECHONL)
	@echo You can also specify RELEASE=debug, RELEASE=noopt, or RELEASE=release
	@echo The default is RELEASE=release

clobber_all :: clean clobber_install

clobber_install ::
	-$(DELTREE) $(INSTALL)

test :: default
	$(MAKE) -C $(APRTEST) -f NWGNUmakefile RELEASE=$(RELEASE) DEST="$(INSTALL)" LM_LICENSE_FILE="$(LM_LICENSE_FILE)"

#
# build recursive targets
#

$(SUBDIRS) : FORCE
ifneq "$(MAKECMDGOALS)" "clean"
	@$(ECHONL)
	@echo Building $(CURDIR)/$@
endif
	$(MAKE) -C $@ $(MAKECMDGOALS) -f NWGNUmakefile RELEASE=$(RELEASE) DEST="$(INSTALL)" LM_LICENSE_FILE="$(LM_LICENSE_FILE)"
	@$(ECHONL)

FORCE : ;

#
# Standard targets
#

clean :: $(SUBDIRS) $(APRTEST)
	@echo Cleaning up $(CURDIR)
	$(call RMDIR,$(OBJDIR))
	$(call DEL,*.err)
	$(call DEL,*.map)
	$(call DEL,*.tmp)
#	$(call DEL,*.d)

$(OBJDIR) ::
	$(call MKDIR,$@)

