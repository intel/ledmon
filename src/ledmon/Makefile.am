# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2009 Intel Corporation.

sbin_PROGRAMS  = ledmon

ledmon_SOURCES = ledmon.c pidfile.h pidfile.c udev.c udev.h
ledmon_LDADD = ../lib/libledinternal.la ../common/libcommon.la $(LIBUDEV_LIBS)
ledmon_CFLAGS = -I$(top_srcdir)/src/lib/include -I$(top_srcdir)/src -I$(top_srcdir)/config \
        -I$(top_srcdir)/src/lib $(AM_CFLAGS) $(LIBUDEV_CFLAGS)
