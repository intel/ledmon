# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Red Hat Inc.

# Simple tests for ledctl.  This needs to be expanded.

import pytest
import logging
from ledctl_cmd import LedctlCmd

LOGGER = logging.getLogger(__name__)

# Note:
# RH SES lab hardware has some enclosures which don't work, which includes the ses tools from sg_utils
# and libStorageMgmt, so it's not something specific with ledmon project.
#
# To run this test do `pytest tests --ledctl-binary=../src/ledctl --slot-filters=sg3-,sg2-`
# to filter out enclosures that don't work

def test_non_slot_set_path(ledctl_binary, slot_filters):
    """
    Test setting the led status by using non-slot syntax, eg. ledctl locate=/dev/sda
    """

    cmd = LedctlCmd(ledctl_binary, slot_filters)
    slots_with_device_nodes = [s for s in cmd.get_all_slots() if s.device_node is not None]

    if len(slots_with_device_nodes) == 0:
        pytest.skip("This test requires slots with devices but none found.")

    for slot in slots_with_device_nodes:
        for state in ["failure", "locate", "normal"]:
            cmd.set_ibpi(slot.device_node, state)
            cur = cmd.get_slot(slot)
            assert cur.state == state,\
                f"unable to set from \"{slot}\" to \"{state}\", current = \"{cur}\" using non-slot syntax"

def test_slot_state_walk(ledctl_binary, slot_filters):
    """
    Test that we can set slots to different states and verify that they reported a change
    """

    cmd = LedctlCmd(ledctl_binary, slot_filters)
    slots = cmd.get_all_slots()

    if len(slots) == 0:
        pytest.skip("This test requires any slot but none found.")

    for slot in slots:
        for state in ["locate", "failure", "normal"]:
            cmd.set_slot_state(slot, state)
            cur = cmd.get_slot(slot)
            assert cur.state == state, f"unable to set from \"{slot}\" to \"{state}\", current = \"{cur}\""
