# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Intel Corporation.

AM_CPPFLAGS = -I@srcdir@./src/lib/include

EXTRA_DIST=lib_unit_test.c

if WITH_TEST
all: lib_unit_test

check_PROGRAMS = lib_unit_test
lib_unit_test_CFLAGS = $(AM_CFLAGS) $(LIBCHECK_CFLAGS)
lib_unit_test_LDADD = ../src/lib/libled.la $(LIBCHECK_LIBS)
lib_unit_test_SOURCES = lib_unit_test.c
endif
