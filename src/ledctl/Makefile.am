# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2009 Intel Corporation.

sbin_PROGRAMS  = ledctl
ledctl_SOURCES = ledctl.c help.c help.h

ledctl_LDADD = ../lib/libledinternal.la ../common/libcommon.la
ledctl_CFLAGS = -I$(top_srcdir)/src/lib/include -I$(top_srcdir)/src -I$(top_srcdir)/config \
        -I$(top_srcdir)/src/lib $(AM_CFLAGS) $(ENABLE_TEST)
