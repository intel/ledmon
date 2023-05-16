#
#  Intel(R) Enclosure LED Utilities
#  Copyright (C) 2009-2023 Intel Corporation.
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

CLEANFILES = ledmon.conf.gz ledmon.gz ledctl.gz
EXTRA_DIST = ledmon.conf.pod ledmon.pod ledctl.pod

dist_man5_MANS = ledmon.conf.gz
dist_man8_MANS = ledmon.gz ledctl.gz

ledmon.conf.gz: ledmon.conf.pod
	pod2man -r "LEDMON.CONF Version $(PACKAGE_VERSION) $(BUILD_LABEL)" -d "@PACKAGE_DATE@" \
		-s 5 -n ledmon.conf -c "Intel(R) Enclosure LED Utilities Config" $< | gzip -9f > $@

ledmon.gz: ledmon.pod
	pod2man -r "LEDMON Version $(PACKAGE_VERSION) $(BUILD_LABEL)" -d "@PACKAGE_DATE@" \
		-s 8 -n ledmon -c "Intel(R) Enclosure LED Monitor Service" $< | gzip -9f > $@

ledctl.gz: ledctl.pod
	pod2man -r "LEDCTL Version $(PACKAGE_VERSION) $(BUILD_LABEL)" -d "@PACKAGE_DATE@" \
		-s 8 -n ledctl -c "Intel(R) Enclosure LED Control Application" $< | gzip -9f > $@
