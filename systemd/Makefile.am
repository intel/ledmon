# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2009 Intel Corporation.

# Installation directory of ledmon systemd service unit.
systemddir = @SYSTEMD_PATH@
SED = sed

CLEANFILES = ledmon.service ledmon.service.tmp

systemd_DATA = ledmon.service

ledmon.service : ledmon.service.in
	$(SED) -e 's|@sbindir[@]|$(sbindir)|g' < $< > $@.tmp
	mv $@.tmp $@
