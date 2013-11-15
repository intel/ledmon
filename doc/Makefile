#
#  Intel(R) Enclosure LED Utilities
#  Copyright (C) 2009-2013 Intel Corporation.
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
MAN_INSTDIR=$(DESTDIR)/usr/share/man
MAN8_INSTDIR=$(MAN_INSTDIR)/man8
MAN5_INSTDIR=$(MAN_INSTDIR)/man5

RELEASE_DATE?="November 2013"
RELEASE_VER?=0.79

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
