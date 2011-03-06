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
	@echo $(DS)targets for RELEASE=$(RELEASE):$(DS)
	@echo $(DS)(default) . . . . libs nlms$(DS)
	@echo $(DS)all . . . . . . . does everything (libs nlms install)$(DS)
	@echo $(DS)libs. . . . . . . builds all libs$(DS)
	@echo $(DS)nlms. . . . . . . builds all nlms$(DS)
	@echo $(DS)install . . . . . builds libs and nlms and copies install files to$(DS)
	@echo $(DS)                  "$(INSTALL)"$(DS)
	@echo $(DS)installdev. . . . copies headers and files needed for development to$(DS)
	@echo $(DS)                  "$(INSTALL)"$(DS)
	@echo $(DS)clean . . . . . . deletes $(OBJDIR) dirs, *.err, and *.map$(DS)
	@echo $(DS)clobber_all . . . deletes all possible output from the make$(DS)
	@echo $(DS)clobber_install . deletes all files in $(INSTALL)$(DS)
	@$(ECHONL)
	@echo $(DS)Multiple targets can be used on a single make command line -$(DS)
	@echo $(DS)(i.e. $(MAKE) clean all)$(DS)
	@$(ECHONL)
	@echo $(DS)You can also specify RELEASE=debug, RELEASE=noopt, or RELEASE=release$(DS)
	@echo $(DS)The default is RELEASE=release$(DS)

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

