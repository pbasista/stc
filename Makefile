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
PNAME := stc

# Kernel name as returned by "uname -s"
KNAME := $(shell uname -s)

# A flag indicating whether the xz compression utility is not available
XZ_UNAVAILABLE := $(shell hash xz 2>/dev/null || echo "COMMAND_UNAVAILABLE")

# The version of the tar program present in the current system.
TAR_VERSION := $(shell tar --version | head -n 1 | cut -d ' ' -f 1)

export CC := gcc
export DOXYGEN := doxygen
export APREFIX := $(PNAME)

P1NAME := st
P2NAME := stsw
E1NAME := $(P1NAME)
E2NAME := $(P2NAME)
ARCHIVE_NC := $(PNAME).tar
ARCHIVE_GZ := $(ARCHIVE_NC).gz
ARCHIVE_XZ := $(ARCHIVE_NC).xz

COMMON_DIR := common
HDRDIR := h
COMMON_HDRDIR := $(COMMON_DIR)/$(HDRDIR)
SRCDIR := src
COMMON_SRCDIR := $(COMMON_DIR)/$(SRCDIR)
OBJDIR := obj
COMMON_OBJDIR := $(COMMON_DIR)/$(OBJDIR)
DEPDIR := d
COMMON_DEPDIR := $(COMMON_DIR)/$(DEPDIR)

COMMON_HEADERS := $(wildcard $(COMMON_HDRDIR)/*.h)
COMMON_SOURCES := $(wildcard $(COMMON_SRCDIR)/*.c)
COMMON_OBJECTS := $(addprefix $(COMMON_OBJDIR)/,\
	$(notdir $(COMMON_SOURCES:.c=.o)))
COMMON_DEPENDENCIES := $(addprefix $(COMMON_DEPDIR)/,\
	$(notdir $(COMMON_SOURCES:.c=.d)))

# This date format almost conforms to the RFC 3339
TIMESTAMP := $(shell date -u "+%Y-%m-%d %H:%M:%S")
OTHERFILES := Makefile Doxyfile README KNOWN_ISSUES

.PHONY: $(P1NAME) $(P2NAME) $(ARCHIVE_NC) \
	doc dist distnc distgz distxz timedist clean distclean

# First and the default target

all: $(P1NAME) $(P2NAME)
	@echo "all projects have been made"

$(P1NAME):
	@$(MAKE) -$(MAKEFLAGS) -C $(P1NAME)

$(P2NAME):
	@$(MAKE) -$(MAKEFLAGS) -C $(P2NAME)

doc:
	@echo "building the documentation"
	@$(DOXYGEN)
	@echo "all the documentation has been built"

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
	@$(MAKE) -$(MAKEFLAGS) -C $(P1NAME) distnc
	@$(MAKE) -$(MAKEFLAGS) -C $(P2NAME) distnc
	@rm -rvf $(COMMON_DEPDIR).tmp
	@mv -v $(COMMON_DEPDIR) $(COMMON_DEPDIR).tmp
	@mkdir -vp $(COMMON_DEPDIR)
	@echo "creating the non-compressed archive $(ARCHIVE_NC)"
	@tar -s '|^|$(APREFIX)/|' -cvf '$(ARCHIVE_NC)' \
		$(COMMON_HEADERS) $(COMMON_SOURCES) $(COMMON_DEPDIR) \
		$(OTHERFILES)
	@rmdir $(COMMON_DEPDIR)
	@mv -v $(COMMON_DEPDIR).tmp $(COMMON_DEPDIR)
	@tar -rvf '$(ARCHIVE_NC)' '@$(P1NAME)/$(P1NAME).tar'
	@rm -v '$(P1NAME)/$(P1NAME).tar'
	@tar -rvf '$(ARCHIVE_NC)' '@$(P2NAME)/$(P2NAME).tar'
	@rm -v '$(P2NAME)/$(P2NAME).tar'
else
$(ARCHIVE_NC):
	@$(MAKE) -$(MAKEFLAGS) -C $(P1NAME) distnc
	@$(MAKE) -$(MAKEFLAGS) -C $(P2NAME) distnc
	@mv -vT $(COMMON_DEPDIR) $(COMMON_DEPDIR).tmp
	@mkdir -vp $(COMMON_DEPDIR)
	@echo "creating the non-compressed archive $(ARCHIVE_NC)"
	@tar --transform 's|^|$(APREFIX)/|' -cvf '$(ARCHIVE_NC)' \
		$(COMMON_HEADERS) $(COMMON_SOURCES) $(COMMON_DEPDIR) \
		$(OTHERFILES)
	@mv -vT $(COMMON_DEPDIR).tmp $(COMMON_DEPDIR)
	@tar -Avf '$(ARCHIVE_NC)' '$(P1NAME)/$(P1NAME).tar'
	@rm -v '$(P1NAME)/$(P1NAME).tar'
	@tar -Avf '$(ARCHIVE_NC)' '$(P2NAME)/$(P2NAME).tar'
	@rm -v '$(P2NAME)/$(P2NAME).tar'
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
	@$(MAKE) -$(MAKEFLAGS) -C $(P1NAME) clean
	@$(MAKE) -$(MAKEFLAGS) -C $(P2NAME) clean
	@rm -vf $(COMMON_DEPENDENCIES) $(COMMON_OBJECTS)
	@echo "all projects have been cleaned"

distclean:
	@rm -vf $(ARCHIVE_NC) $(ARCHIVE_GZ) $(ARCHIVE_XZ)
	@echo "distribution archives cleaned"
