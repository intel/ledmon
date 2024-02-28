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

SUBDIRS = include

LIB_SRCS         = ahci.c block.c cntrl.c enclosure.c utils.c list.c \
                   raid.c scsi.c tail.c sysfs.c smp.c dellssd.c \
                   pci_slot.c vmdssd.c amd.c amd_sgpio.c amd_ipmi.c\
                   ipmi.c npem.c ses.c slot.c \
                   ahci.h amd_sgpio.h block.h cntrl.h dellssd.h utils.h \
                   enclosure.h list.h pci_slot.h raid.h scsi.h \
                   ses.h tail.h smp.h status.h sysfs.h \
                   vmdssd.h ipmi.h amd.h amd_ipmi.h npem.h libled_internal.c \
		   slot.h libled_private.h libled_internal.h

# Make a convenience library, to be used for led tools and the shared library
noinst_LTLIBRARIES = libledinternal.la
libledinternal_la_SOURCES = libled.c $(LIB_SRCS)
libledinternal_la_LIBADD = $(LIBPCI_LIBS) ../common/libcommon.la
libledinternal_la_CFLAGS = -I$(top_srcdir)/src/lib/include -I$(top_srcdir)/src \
        -I$(top_srcdir)/config $(AM_CFLAGS) $(AM_LIB_CFLAGS) $(LIBPCI_CFLAGS)

# User may not want the shared library
if WITH_LIBRARY

lib_LTLIBRARIES = libled.la
libled_la_SOURCES =
libled_la_LIBADD = libledinternal.la
libled_la_LDFLAGS = -version-info $(LIBLED_LIBTOOL_VERSION)

endif
