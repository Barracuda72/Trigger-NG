#
# Trigger Rally GNUmakefile for Linux users
#

# standard GNU and custom variables
DISTNAME        := trigger-rally
DISTVER         := 0.6.7
DISTDIR         := $(DISTNAME)-$(DISTVER)
DISTARC         := $(DISTDIR).tar.gz
TR_EXENAME      := trigger-rally
TR_CFGNAME      := trigger-rally.config.defs
TR_BINDIR       := ../bin
TR_DATADIR      := ../data
TR_DOCDIR       := ../doc
TR_EXEFILE      := $(TR_BINDIR)/$(TR_EXENAME)
TR_CFGFILE      := $(TR_BINDIR)/$(TR_CFGNAME)
TR_DESKTOPNAME  := trigger-rally.desktop
TR_APPDATANAME  := trigger-rally.appdata.xml
TR_DESKTOPFILE  := $(TR_DATADIR)/icon/$(TR_DESKTOPNAME)
TR_APPDATAFILE  := $(TR_DATADIR)/icon/$(TR_APPDATANAME)
TR_DATAFILES    := $(shell cd $(TR_DATADIR); find * -type f)
TR_DOCFILES     := $(shell cd $(TR_DOCDIR); find * -type f)
LOC_DESKTOPFILE := /usr/share/applications
LOC_APPDATAFILE := /usr/share/metainfo
PROJDIRS        := PEngine PSim Trigger
SRCFILES        := $(sort $(shell find $(PROJDIRS) -type f -name "*.cpp"))
OBJFILES        := $(patsubst %.cpp, %.o, $(SRCFILES))
DEPFILES        := $(patsubst %.cpp, %.d, $(SRCFILES))
WARNINGS        ?= -Wall -Wextra -pedantic
OPTIMS          ?= -march=native -mtune=native -Ofast
DMACROS         := -DNDEBUG -DUNIX -DPACKAGE_VERSION=\"$(DISTVER)\"
INCDIRS         := -I'./include' -I'../glm'
CXXFLAGS        += -std=c++11 $(WARNINGS) $(OPTIMS) -g
CPPFLAGS        += $(DMACROS) $(INCDIRS)
EXTRA_LIBS      := -lSDL2main -lSDL2 -lSDL2_image -lphysfs -lopenal -lalut -lpthread -ltinyxml2
INSTALL_PROGRAM := install --mode=0755
INSTALL_DATA    := install --mode=0644

ifeq ($(GLES2),1)
DMACROS         += -DGLES2
EXTRA_LIBS      += -lGLESv2
else # !GLES2
EXTRA_LIBS      += -lGL -lGLEW

ifeq ($(GL30PLUS),1)
DMACROS         += -DGL30PLUS
endif # GL30PLUS

endif # GLES2

LDFLAGS         += $(EXTRA_LIBS)

# standard GNU directories
prefix          ?= /usr/local
exec_prefix     ?= $(prefix)
bindir          := $(exec_prefix)/games
datarootdir     := $(prefix)/share
datadir         := $(datarootdir)/games
docdir          := $(datarootdir)/doc/$(DISTNAME)

#
# phony targets, whose names aren't names of resulting files
#
# NOTE that several "standard" targets are missing; here
# `all` is `build` because I always felt "all" was a bad name,
# while the others I had either no time or no incentive to implement
#
.PHONY: build printvars install uninstall installdirs dist clean

Debug: build

Release: build

cleanDebug: clean

cleanRelease: clean

# builds the executable
build: printvars $(TR_EXEFILE)

#
# prints the variables that the user can change;
# if VAR is in the list, then the user can change it by running:
#
#   VAR="new value" make [target]
#
# examples:
#   OPTIMS="-O0 -g" make
#   prefix="/usr" exec_prefix="/usr" make install
#
printvars:
	@printf "\ncurrent values of user-set variables:\n"
	@printf "\tDESTDIR      ?= %s\n" "$(DESTDIR)"
	@printf "\tprefix       ?= %s\n" "$(prefix)"
	@printf "\texec_prefix  ?= %s\n" "$(exec_prefix)"
	@printf "\tOPTIMS       ?= %s\n" "$(OPTIMS)"
	@printf "\tWARNINGS     ?= %s\n" "$(WARNINGS)"
	@printf "\n"
	@printf "resulting values of build variables:\n"
	@printf "\tCXXFLAGS     += %s\n" "$(CXXFLAGS)"
	@printf "\tCPPFLAGS     += %s\n" "$(CPPFLAGS)"
	@printf "\tLDFLAGS      += %s\n" "$(LDFLAGS)"
	@printf "\n"

# installs the software (executable, data files and documentation)
install: installdirs build
	@printf "\ninstall [%s] begin\n" "$(DISTNAME)"
	@$(INSTALL_PROGRAM) "$(TR_EXEFILE)" "$(DESTDIR)$(bindir)"
	@$(INSTALL_DATA) "$(TR_CFGFILE)" "$(DESTDIR)$(bindir)"
	@for file in $(TR_DATAFILES); do \
		$(INSTALL_DATA) -D "$(TR_DATADIR)/$$file" "$(DESTDIR)$(datadir)/$(DISTNAME)/$$file"; \
		done
	@for file in $(TR_DOCFILES); do \
		$(INSTALL_DATA) -D "$(TR_DOCDIR)/$$file" "$(DESTDIR)$(docdir)/$$file"; \
		done
	@$(INSTALL_DATA) "$(TR_DESKTOPFILE)" "$(DESTDIR)$(LOC_DESKTOPFILE)"
	@$(INSTALL_DATA) "$(TR_APPDATAFILE)" "$(DESTDIR)$(LOC_APPDATAFILE)"
	@printf "install [%s] end\n\n" "$(DISTNAME)"

# deletes the software (executable, data files and documentation)
uninstall:
	-@$(RM) --verbose "$(DESTDIR)$(bindir)/$(TR_EXENAME)"
	-@$(RM) --verbose "$(DESTDIR)$(bindir)/$(TR_CFGNAME)"
	-@$(RM) --verbose --recursive "$(DESTDIR)$(datadir)/$(DISTNAME)"
	-@$(RM) --verbose --recursive "$(DESTDIR)$(docdir)"
	-@$(RM) --verbose "$(DESTDIR)$(LOC_DESKTOPFILE)/$(TR_DESKTOPNAME)"
	-@$(RM) --verbose "$(DESTDIR)$(LOC_APPDATAFILE)/$(TR_APPDATANAME)"

# creates the installation directories if they do not exist
installdirs:
	@printf "\nmkdir\t[installation directories]\n"
	@mkdir --mode=0755 --parents \
		"$(DESTDIR)$(bindir)" \
		"$(DESTDIR)$(datadir)/$(DISTNAME)" \
		"$(DESTDIR)$(docdir)"

#
# creates a zipped tarball for redistribution
#
# NOTE that this fails if the directory we're in isn't named $(DISTDIR) however
# this is by design: it's meant to enforce directory naming consistency and
# awareness of the $(DISTVER) variable, which you must update for each release!
#
dist: clean
	@printf "\ntar -> \"../../%s\"\n" "$(DISTARC)"
	@cd ../..; \
		tar --owner=root --group=root --create --file="$(DISTARC)" --auto-compress "$(DISTDIR)"
	@printf "md5sum -> \"../../%s.md5\"\n\n" "$(DISTARC)"
	@cd ../..; \
		md5sum "$(DISTARC)" > "$(DISTARC).md5"

# links the object files into the executable
$(TR_EXEFILE): $(OBJFILES)
	@printf "%s" $(CXX)
	@for file in $(OBJFILES); do \
		printf "\t%s\n" $$file; \
		done
	@printf "\t-> %s\n" $@
	@$(CXX) -o $@ $(OBJFILES) $(LDFLAGS)

#
# removes object files, dependency files, executable and
# backup files (such as "func.cpp~")
#
clean:
	-@$(RM) --verbose \
		$(OBJFILES) \
		$(DEPFILES) \
		$(TR_EXEFILE) \
		$(shell find -type f -name "*~")

#
# includes dependency files; these files are needed to speed up
# the build process by excluding the compilation of object files
# which are already up-to-date
#
-include $(DEPFILES)

#
# compiles source files to object files
#
# GNUmakefile was added as a dependency in order to trigger a rebuild
# if this makefile is edited
#
%.o: %.cpp GNUmakefile
	@printf "%s\t%s -> %s\n" $(CXX) $< $@
	@$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@
