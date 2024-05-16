# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2009 Intel Corporation.

# Intel(R) Enclosure LED Utilities

CLEANFILES = ledmon.conf.gz ledmon.gz ledctl.gz
EXTRA_DIST = ledmon.conf.pod ledmon.pod ledctl.pod

dist_man5_MANS = ledmon.conf.gz
dist_man8_MANS = ledmon.gz ledctl.gz

ledmon.conf.gz: ledmon.conf.pod
	pod2man -r "LEDMON.CONF Version $(PACKAGE_VERSION) $(BUILD_LABEL)" -d "@PACKAGE_DATE@" \
		-s 5 -n ledmon.conf -c "Intel(R) Enclosure LED Utilities Config" $< | gzip -9f > $@

ledmon.gz: ledmon.pod
	pod2man -r "LEDMON Version $(PACKAGE_VERSION) $(BUILD_LABEL)" -d "@PACKAGE_DATE@" \
		-s 8 -n ledmon -c "Intel(R) Enclosure LED Monitor Service" $< | gzip -9f > $@

ledctl.gz: ledctl.pod
	pod2man -r "LEDCTL Version $(PACKAGE_VERSION) $(BUILD_LABEL)" -d "@PACKAGE_DATE@" \
		-s 8 -n ledctl -c "Intel(R) Enclosure LED Control Application" $< | gzip -9f > $@
