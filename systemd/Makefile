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

# Comment out, and uncomment empty if you do not want pkg-config path.
SYSTEMD_SERVICE_INSTDIR=$(shell pkg-config --variable=systemdsystemunitdir systemd)
#SYSTEMD_SERVICE_INSTDIR=
ifeq ($(SYSTEMD_SERVICE_INSTDIR),)
DESTDIR?=
SYSTEMD_SERVICE_INSTDIR=$(DESTDIR)/etc/systemd/system
endif

default:

uninstall:
	rm -f $(SYSTEMD_SERVICE_INSTDIR)/ledmon.service

install:
	install -D ./ledmon.service $(SYSTEMD_SERVICE_INSTDIR)/ledmon.service

