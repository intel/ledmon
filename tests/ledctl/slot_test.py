# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Red Hat Inc.

# The purpose of this file is testing interactions with hardware. Tests are projected
# to be executed only if possible (required hardware is available).
# Ledctl support many blinking standards, there is no possibility to test them all at once.
# We parametrize each test by controller type to gather logs confirming that it was run
# on appropriative hardware.
# Collecting such results will be benefitable for release stuff, it gives some creditability that it
# was minimally tested.

import pytest
import logging
from ledctl_cmd import LedctlCmd

LOGGER = logging.getLogger(__name__)


def get_slots_with_device_or_skip(cmd: LedctlCmd, cntrl):
    try:
        slots_with_device_node = cmd.get_slots_with_device(cntrl)
    except AssertionError as e:
        pytest.skip(str(e))

    if len(slots_with_device_node) == 0:
        pytest.skip("No slot with device found")
    return slots_with_device_node


@pytest.mark.parametrize("cntrl", LedctlCmd.slot_mgmt_ctrls)
def test_ibpi(ledctl_binary, slot_filters, controller_filters, cntrl):
    """
    Test setting the led status by using IBPI syntax for the disk under the chosen controller.
    Limited to controllers with slots feature support.
    """

    cmd = LedctlCmd(ledctl_binary, slot_filters, controller_filters)
    slots_with_device_node = get_slots_with_device_or_skip(cmd, cntrl)

    for slot in slots_with_device_node:
        for state in LedctlCmd.base_states:
            cmd.set_ibpi(slot.device_node, state)
            cur = cmd.get_slot(slot)
            assert cur.state == state,\
                f"unable to set \"{slot.device_node}\" to \"{state}\", current = \"{cur}\" using ibpi syntax"


@pytest.mark.parametrize("cntrl", LedctlCmd.slot_mgmt_ctrls)
def test_set_slot_by_slot(ledctl_binary, slot_filters, controller_filters,
                          cntrl):
    """
    Test that we can set slots to different states and verify that they reported a change, using --slot.
    """

    cmd = LedctlCmd(ledctl_binary, slot_filters, controller_filters)
    try:
        slots = [s for s in cmd.get_slots(cntrl)]
    except AssertionError as e:
        pytest.skip(str(e))

    if len(slots) == 0:
        pytest.skip("No slot found")

    for slot in slots:
        for state in LedctlCmd.base_states:
            cmd.set_slot_state(slot, state)
            cur = cmd.get_slot(slot)
            assert cur.state == state,\
                f"unable to set \"{slot}\" to \"{state}\", current = \"{cur}\" using slot"


def slot_set_and_get_by_device_all(cmd: LedctlCmd, slot):
    for state in LedctlCmd.base_states:
        cmd.set_device_state(slot, state)
        cur = cmd.get_slot_by_device(slot)
        assert cur.state == state,\
            f"unable to set \"{slot}\" to \"{state}\", current = \"{cur}\" using --device"


@pytest.mark.parametrize("cntrl", LedctlCmd.slot_mgmt_ctrls)
def test_set_slot_by_device(ledctl_binary, slot_filters, controller_filters,
                            cntrl):
    """
    Test that we can set slots to different states and verify that they reported a change, using device.
    """

    cmd = LedctlCmd(ledctl_binary, slot_filters, controller_filters)
    slots_with_device_node = get_slots_with_device_or_skip(cmd, cntrl)

    for slot in slots_with_device_node:
        slot_set_and_get_by_device_all(cmd, slot)


@pytest.mark.parametrize("cntrl", ["VMD", "NPEM"])
def test_nvme_multipath_drives(ledctl_binary, slot_filters, controller_filters,
                               cntrl):
    """
    Special test for multipath drives using both set methods and get via device. We need to check
    if ledctl provides nvme multipath minimal support.
    """
    cmd = LedctlCmd(ledctl_binary, slot_filters, controller_filters)

    mp_drives = cmd.get_mp_nodes()
    if len(mp_drives) == 0:
        pytest.skip("No nvme multipath drives found")

    slots_with_device_node = get_slots_with_device_or_skip(cmd, cntrl)
    any_found = False

    for slot in slots_with_device_node:
        if slot.device_node not in mp_drives:
            continue
        any_found = True

        LOGGER.debug(f"Found nvme multipath drive {slot}")

        for state in cmd.base_states:
            cmd.set_ibpi(slot.device_node, state)
            cur = cmd.get_slot_by_device(slot)
            assert cur.state == state,\
                f"unable to set \"{slot}\" to \"{state}\", current = \"{cur}\" using ibpi syntax"

        slot_set_and_get_by_device_all(cmd, slot)

    if not any_found:
        pytest.skip("Multipath drives are not connected to tested controller")
