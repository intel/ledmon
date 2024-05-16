# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2009 Intel Corporation.

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

