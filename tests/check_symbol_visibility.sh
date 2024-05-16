#!/usr/bin/bash

# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Red Hat Inc.

# Ensure we only have shared library symbols which start with "led_*"


tmpf=$(mktemp /tmp/ledmon_sym.XXXXXXXX) || exit 1

function cleanup {
        rm  -f "${tmpf}" || exit 1
}

trap cleanup EXIT

nm -D src/lib/.libs/libled.so | grep " T " | grep -v " led_" > "${tmpf}"

found=$(wc -l < "${tmpf}") || exit 1

if [[ ${found} -ne "0" ]]; then
        echo "ERROR: Public symbols found which don't have led_ prefix for shared library"
        cat "${tmpf}"
        exit 1
fi

exit 0
