#
# Intel(R) Enclosure LED Utilities
# Copyright (C) 2009 Intel Corporation. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

# Installation directory
DESTDIR?=

.PHONY: ledmon ledctl man all

default: all

ledmon: 
	$(MAKE) -C src ledmon

ledctl:
	$(MAKE) -C src ledctl

man:
	$(MAKE) -C doc all

all: ledmon ledctl man

install: all
	$(MAKE) -C src DESTDIR=$(DESTDIR) install
	$(MAKE) -C doc DESTDIR=$(DESTDIR) install

uninstall:
	$(MAKE) -C src DESTDIR=$(DESTDIR) uninstall
	$(MAKE) -C doc DESTDIR=$(DESTDIR) uninstall

clean:
	$(MAKE) -C src clean
	$(MAKE) -C doc clean

mrproper: 
	$(MAKE) -C src mrproper
	$(MAKE) -C doc clean
