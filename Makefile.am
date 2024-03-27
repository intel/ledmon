#
#  Intel(R) Enclosure LED Utilities
#  Copyright (C) 2009-2024 Intel Corporation.
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

ACLOCAL_AMFLAGS = -I m4

if SYSTEMD_CONDITION
  OPTIONAL_SUBDIR = systemd
endif

if WITH_DOC
  DOC_SUBDIR = doc
  dist_doc_DATA = README.md
endif

SUBDIRS = src $(DOC_SUBDIR) $(OPTIONAL_SUBDIR) tests
EXTRA_DIST = \
	config/config.h \
	systemd/ledmon.service.in \
	ledmon.pc.in \
	ledmon.pc

if WITH_LIBRARY

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ledmon.pc

endif

if WITH_TEST
TESTS = tests/runtests.sh
endif

