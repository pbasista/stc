# Copyright 2012 Peter Bašista
#
# This file is part of the stc.
#
# stc is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# The name of the project
PNAME := stsw

# if the this is a top-level make invocation
ifeq ($(MAKELEVEL),0)
# we need to define some basic variables
CC := gcc
APREFIX := $(PNAME)
# On the other hand, if this is a nested make invocation,
# we suppose that almost all the necessary variables are already defined.
else
# while some of them, like this one, need to be corrected
APREFIX := $(APREFIX)/$(PNAME)
endif

# Kernel name as returned by "uname -s"
KNAME := $(shell uname -s)

# A flag indicating whether the xz compression utility is not available
XZ_UNAVAILABLE := $(shell hash xz 2>/dev/null || echo "COMMAND_UNAVAILABLE")

# The version of the tar program present in the current system.
TAR_VERSION := $(shell tar --version | head -n 1 | cut -d ' ' -f 1)

COMMON_DIR := ../common
HDRDIR := h
COMMON_HDRDIR := $(COMMON_DIR)/$(HDRDIR)
SRCDIR := src
COMMON_SRCDIR := $(COMMON_DIR)/$(SRCDIR)
OBJDIR := obj
COMMON_OBJDIR := $(COMMON_DIR)/$(OBJDIR)
DEPDIR := d
COMMON_DEPDIR := $(COMMON_DIR)/$(DEPDIR)
ENAME := $(PNAME)
ARCHIVE_NC := $(PNAME).tar
ARCHIVE_GZ := $(ARCHIVE_NC).gz
ARCHIVE_XZ := $(ARCHIVE_NC).xz
CFLAGS := -I$(COMMON_HDRDIR) -I$(HDRDIR)

# If we are on the Mac OS, we would like to link with the iconv
ifeq ($(KNAME),Darwin)
LIBS := -liconv
else
LIBS :=
endif

AFLAGS := -O3 -pthread -std=gnu99 -Wall -Wextra -Wconversion -pedantic -g
COMMON_HEADERS := $(wildcard $(COMMON_HDRDIR)/*.h)
HEADERS := $(wildcard $(HDRDIR)/*.h)
COMMON_SOURCES := $(wildcard $(COMMON_SRCDIR)/*.c)
SOURCES := $(wildcard $(SRCDIR)/*.c)
COMMON_OBJECTS := $(addprefix $(COMMON_OBJDIR)/,\
	$(notdir $(COMMON_SOURCES:.c=.o)))
OBJECTS := $(addprefix $(OBJDIR)/,$(notdir $(SOURCES:.c=.o)))
COMMON_DEPENDENCIES := $(addprefix $(COMMON_DEPDIR)/,\
	$(notdir $(COMMON_SOURCES:.c=.d)))
DEPENDENCIES := $(addprefix $(DEPDIR)/,$(notdir $(SOURCES:.c=.d)))

# This date format almost conforms to the RFC 3339
TIMESTAMP := $(shell date -u "+%Y-%m-%d %H:%M:%S")
OTHERFILES := Makefile

.PHONY: $(ARCHIVE_NC) dist distnc distgz distxz timedist clean cleanall

# First and the default target

all: $(COMMON_DEPENDENCIES) $(DEPENDENCIES) \
	$(COMMON_OBJDIR) $(OBJDIR) \
	$(COMMON_OBJECTS) $(OBJECTS) $(ENAME)
	@echo "$(PNAME) has been made"

$(COMMON_DEPENDENCIES): $(COMMON_DEPDIR)/%.d: $(COMMON_SRCDIR)/%.c
	@echo "DEP $@"
	@$(CC) -MM -MT \
		'$@ $(addprefix $(COMMON_OBJDIR)/,\
		$(subst .c,.o,$(notdir $<)))' \
		$(CFLAGS) $(AFLAGS) $< -o $@

$(DEPENDENCIES): $(DEPDIR)/%.d: $(SRCDIR)/%.c
	@echo "DEP $@"
	@$(CC) -MM -MT \
		'$@ $(addprefix $(OBJDIR)/,$(subst .c,.o,$(notdir $<)))' \
		$(CFLAGS) $(AFLAGS) $< -o $@

include $(COMMON_DEPENDENCIES)
include $(DEPENDENCIES)

$(COMMON_OBJDIR):
	@echo "creating common object directory (requested by $(PNAME))"
	@mkdir $(COMMON_OBJDIR)

$(OBJDIR):
	@echo "creating object directory for $(PNAME)"
	@mkdir $(OBJDIR)

$(COMMON_OBJECTS) $(OBJECTS):
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) $(AFLAGS) $< -o $@

$(ENAME): $(COMMON_OBJECTS) $(OBJECTS)
	@echo "LD $(ENAME)"
	@$(CC) $(LIBS) $(AFLAGS) $(COMMON_OBJECTS) $(OBJECTS) -o $(ENAME)

# If we do not have the xz compression utility,
# we would like to use the gzip instead
ifeq ($(XZ_UNAVAILABLE),COMMAND_UNAVAILABLE)
dist: distgz
else
dist: distxz
endif

# If the available tar version is bsd tar, we have to change
# the syntax of the transformation command
ifeq ($(TAR_VERSION),bsdtar)
$(ARCHIVE_NC):
	@rm -rvf $(DEPDIR).tmp
	@mv -v $(DEPDIR) $(DEPDIR).tmp
	@mkdir -vp $(DEPDIR)
	@echo "creating the non-compressed archive $(ARCHIVE_NC)"
	@tar -s '|^|$(APREFIX)/|' -cvf '$(ARCHIVE_NC)' \
		$(HEADERS) $(SOURCES) $(DEPDIR) $(OTHERFILES)
	@rmdir $(DEPDIR)
	@mv -v $(DEPDIR).tmp $(DEPDIR)
else
$(ARCHIVE_NC):
	@rm -rvf $(DEPDIR).tmp
	@mv -v $(DEPDIR) $(DEPDIR).tmp
	@mkdir -vp $(DEPDIR)
	@echo "creating the non-compressed archive $(ARCHIVE_NC)"
	@tar --transform 's|^|$(APREFIX)/|' -cvf '$(ARCHIVE_NC)' \
		$(HEADERS) $(SOURCES) $(DEPDIR) $(OTHERFILES)
	@rmdir $(DEPDIR)
	@mv -v $(DEPDIR).tmp $(DEPDIR)
endif

$(ARCHIVE_GZ): $(ARCHIVE_NC)
	@echo "compressing the archive"
	@gzip -v '$(ARCHIVE_NC)'

$(ARCHIVE_XZ): $(ARCHIVE_NC)
	@echo "compressing the archive"
	@xz -v '$(ARCHIVE_NC)'

distnc: $(ARCHIVE_NC)
	@echo "archive $(ARCHIVE_NC) created"

distgz: $(ARCHIVE_GZ)
	@echo "archive $(ARCHIVE_GZ) created"

distxz: $(ARCHIVE_XZ)
	@echo "archive $(ARCHIVE_XZ) created"

timedist: $(ARCHIVE_NC)
	@echo "renaming the archive"
	@mv '$(ARCHIVE_NC)' '$(TIMESTAMP) $(ARCHIVE_NC)'
	@echo "archive '$(TIMESTAMP) $(ARCHIVE_NC)' created"

clean:
	@rm -vf $(DEPENDENCIES) $(OBJECTS) $(ENAME)
	@echo "$(PNAME) cleaned"

cleanall: clean
	@rm -vf $(COMMON_DEPENDENCIES) $(COMMON_OBJECTS)
	@echo "common objects and dependencies cleaned (requested by $(PNAME))"
