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

# Make a convenience library for this code
noinst_LTLIBRARIES = libcommon.la
libcommon_la_SOURCES = config_file.h config_file.c
libcommon_la_CFLAGS = -I$(top_srcdir)/src/lib/include -I$(top_srcdir)/src -I$(top_srcdir)/config \
        $(AM_CFLAGS) $(AM_LIB_CFLAGS) $(LIBPCI_CFLAGS)
