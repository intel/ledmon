#
#  Intel(R) Enclosure LED Utilities
#  Copyright (C) 2009-2016 Intel Corporation.
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

# Installation directory prefix.
DESTDIR?=

# Installation directory of ledctl application.
LEDCTL_INSTDIR=$(DESTDIR)/usr/sbin

# Installation directory of ledmon application.
LEDMON_INSTDIR=$(DESTDIR)/usr/sbin

# Output directory for object files.
OUTDIR?=$(shell pwd)/_build

# Build components common to both of applications.
OBJECTS=\
	$(OUTDIR)/ahci.o	\
	$(OUTDIR)/block.o	\
	$(OUTDIR)/cntrl.o	\
	$(OUTDIR)/enclosure.o	\
	$(OUTDIR)/list.o	\
	$(OUTDIR)/raid.o	\
	$(OUTDIR)/scsi.o	\
	$(OUTDIR)/slave.o	\
	$(OUTDIR)/status.o	\
	$(OUTDIR)/sysfs.o	\
	$(OUTDIR)/smp.o		\
	$(OUTDIR)/dellssd.o	\
	$(OUTDIR)/utils.o	\
	$(OUTDIR)/pci_slot.o	\
        $(OUTDIR)/vmdssd.o      \
        $(OUTDIR)/udev.o

# Build components of ledmon application.
LEDMON_OBJECTS=\
	$(OUTDIR)/ledmon.o  	\
	$(OUTDIR)/pidfile.o	\
	$(OBJECTS)

# Build components of ledctl application.
LEDCTL_OBJECTS=\
	$(OUTDIR)/ledctl.o	\
	$(OBJECTS)

CXFLAGS=-O0 -g
CWFLAGS=-Wall
CFLAGS=$(CXFLAGS) $(CWFLAGS)

DEFFLAGS=-D_DEBUG -D_GNU_SOURCE -D_BSD_SOURCE -DDMALLOC_DISABLE
CPPFLAGS=$(DEFFLAGS)
ALL_CPPFLAGS=$(CPPFLAGS) -I../config

LDFLAGS=$(CXFLAGS)
LDLIBS=-lsgutils2 -ludev

SOURCES:=$(shell find -name "*.[cC]" -print)

default: all

ledmon: $(OUTDIR)/.depend $(LEDMON_OBJECTS)
	$(CC) $(LDFLAGS) $(LEDMON_OBJECTS) $(LDLIBS) -o $@

ledctl: $(OUTDIR)/.depend $(LEDCTL_OBJECTS)
	$(CC) $(LDFLAGS) $(LEDCTL_OBJECTS) $(LDLIBS) -o $@

$(OUTDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(ALL_CPPFLAGS) -c $< -o $@

all: $(OUTDIR)/.depend ledmon ledctl

uninstall:
	rm -f $(LEDMON_INSTDIR)/ledmon
	rm -f $(LEDCTL_INSTDIR)/ledctl

install: all
	install -D ./ledmon $(LEDMON_INSTDIR)/ledmon
	install -D ./ledctl $(LEDCTL_INSTDIR)/ledctl

clean:
	rm -rf $(OUTDIR)

depend dep: $(OUTDIR)/.depend

mrproper: clean
	rm -f ledmon ledctl

$(OUTDIR)/.depend: $(SOURCES)
	mkdir -p $(OUTDIR)
	$(CC) $(CFLAGS) $(ALL_CPPFLAGS) -M $^ | sed 's/^[a-z,0-9]/$(subst /,\/,$(OUTDIR))\/&/' > $(OUTDIR)/.depend

ifeq ($(filter install uninstall depend clean mrproper,$(MAKECMDGOALS)),)
    -include $(OUTDIR)/.depend
endif
