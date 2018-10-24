#
#  Intel(R) Enclosure LED Utilities
#  Copyright (C) 2009-2017 Intel Corporation.
#
#  This program is free software; you can redistribute it and/or modify it
#  under the terms and conditions of the GNU General Public License,
#  version 2, as published by the Free Software Foundation.
#
#  This program is distributed in the hope it will be useful, but WITHOUT
#  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
#  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
#  more details.
#
#  You should have received a copy of the GNU General Public License along with
#  this program; if not, write to the Free Software Foundation, Inc., 
#  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
#

# Installation directory
DESTDIR?=

.PHONY: ledmon ledctl man all

default: all

ledmon: 
	$(MAKE) -C src BUILD_LABEL="$(BUILD_LABEL)" ledmon

ledctl:
	$(MAKE) -C src BUILD_LABEL="$(BUILD_LABEL)" ledctl

man:
	$(MAKE) -C doc BUILD_LABEL="$(BUILD_LABEL)" all

all: ledmon ledctl man

install: all
	$(MAKE) -C src DESTDIR=$(DESTDIR) install
	$(MAKE) -C doc DESTDIR=$(DESTDIR) install

install-systemd:
	$(MAKE) -C systemd DESTDIR=$(DESTDIR) install

uninstall:
	$(MAKE) -C src DESTDIR=$(DESTDIR) uninstall
	$(MAKE) -C doc DESTDIR=$(DESTDIR) uninstall
	$(MAKE) -C systemd DESTDIR=$(DESTDIR) uninstall

clean:
	$(MAKE) -C src clean
	$(MAKE) -C doc clean

mrproper: 
	$(MAKE) -C src mrproper
	$(MAKE) -C doc clean

TAGS_FILES=$(shell ls Makefile src/Makefile */*.[ch])
TAGS: $(TAGS_FILES)
	@etags $(TAGS_FILES)
