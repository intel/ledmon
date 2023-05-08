#
#  Intel(R) Enclosure LED Utilities
#  Copyright (C) 2009-2022 Intel Corporation.
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

COMMON_SRCS      = ahci.c block.c cntrl.c config_file.c enclosure.c list.c \
                   raid.c scsi.c slave.c sysfs.c smp.c dellssd.c \
                   utils.c pci_slot.c vmdssd.c udev.c amd.c amd_sgpio.c amd_ipmi.c\
                   ipmi.c npem.c ses.c slot.c \
                   ahci.h amd_sgpio.h block.h cntrl.h config_file.h dellssd.h \
                   enclosure.h ibpi.h list.h pci_slot.h pidfile.h raid.h scsi.h \
                   ses.h slave.h smp.h status.h sysfs.h udev.h utils.h \
                   vmdssd.h ipmi.h amd.h amd_ipmi.h npem.h slot.h

LEDMON_SRCS      = ledmon.c pidfile.c $(COMMON_SRCS)
LEDCTL_SRCS      = ledctl.c $(COMMON_SRCS)


sbin_PROGRAMS  = ledmon ledctl
ledmon_SOURCES = $(LEDMON_SRCS)
ledctl_SOURCES = $(LEDCTL_SRCS)

ledmon_LDADD = $(LIBPCI_LIBS) $(LIBUDEV_LIBS)
ledctl_LDADD = $(LIBPCI_LIBS) $(LIBUDEV_LIBS)

ledmon_CFLAGS = $(AM_CFLAGS) $(LIBPCI_CFLAGS) $(LIBUDEV_CFLAGS)
ledctl_CFLAGS = $(AM_CFLAGS) $(LIBPCI_CFLAGS) $(LIBUDEV_CFLAGS)
