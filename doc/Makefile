#
# Intel(R) Enclosure LED Control Utilities
# Copyright (C) 2009,2011 Intel Corporation. All rights reserved.
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
MAN_INSTDIR=$(DESTDIR)/usr/share/man
MAN8_INSTDIR=$(MAN_INSTDIR)/man8
MAN5_INSTDIR=$(MAN_INSTDIR)/man5

RELEASE_DATE="June 2011"
RELEASE_VER=0.2

default: all

ledctl.conf.5.gz: ledctl.conf.pod
	pod2man -r "LEDCTL.CONF Version $(RELEASE_VER)" -d $(RELEASE_DATE) \
		-s 5 -n ledctl.conf -c "Inter(R) Enclosure LED Utilities Config" $< | gzip -9f > $@

ledmon.8.gz: ledmon.pod
	pod2man -r "LEDMON Version $(RELEASE_VER)" -d $(RELEASE_DATE) \
		-s 8 -n ledmon -c "Intel(R) Enclosure LED Monitor Service" $< | gzip -9f > $@

ledctl.8.gz: ledctl.pod
	pod2man -r "LEDCTL Version $(RELEASE_VER)" -d $(RELEASE_DATE) \
		-s 8 -n ledctl -c "Intel(R) Enclosure LED Control Application" $< | gzip -9f > $@	

install: all
	install -D -m 644 ./ledmon.8.gz $(MAN8_INSTDIR)/ledmon.8.gz
	install -D -m 644 ./ledctl.8.gz $(MAN8_INSTDIR)/ledctl.8.gz
	install -D -m 644 ./ledctl.conf.5.gz $(MAN5_INSTDIR)/ledctl.conf.5.gz

uninstall:
	rm -f $(MAN8_INSTDIR)/ledmon.8.gz $(MAN8_INSTDIR)/ledctl.8.gz
	rm -f $(MAN5_INSTDIR)/ledctl.conf.5.gz

all: ledctl.conf.5.gz ledmon.8.gz ledctl.8.gz

clean:
	rm -f ledmon.8 ledmon.8.gz ledctl.8 ledctl.8.gz
	rm -f ledctl.conf.5 ledctl.conf.5.gz
