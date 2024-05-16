# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2009 Intel Corporation.

# Make a convenience library for this code
noinst_LTLIBRARIES = libcommon.la
libcommon_la_SOURCES = config_file.h config_file.c
libcommon_la_CFLAGS = -I$(top_srcdir)/src/lib/include -I$(top_srcdir)/src -I$(top_srcdir)/config \
        $(AM_CFLAGS) $(AM_LIB_CFLAGS) $(LIBPCI_CFLAGS)
