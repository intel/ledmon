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
#

sbin_PROGRAMS  = ledmon

ledmon_SOURCES = ledmon.c pidfile.h pidfile.c udev.c udev.h
ledmon_LDADD = ../lib/libledinternal.la ../common/libcommon.la $(LIBUDEV_LIBS)
ledmon_CFLAGS = -I$(top_srcdir)/src/lib/include -I$(top_srcdir)/src -I$(top_srcdir)/config \
        -I$(top_srcdir)/src/lib $(AM_CFLAGS) $(LIBUDEV_CFLAGS)
