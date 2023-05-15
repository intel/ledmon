#!/usr/bin/python3

# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Red Hat Inc.

# Simple tests for ledctl.  This needs to be expanded.

import sys
import subprocess
import re

LEDCTL_BIN = "/usr/sbin/ledctl"
SLOT_LINE = re.compile(r"^slot: (.+)led state:(.+)device:(.+)$")

SLOT_FILTERS = []

# Note:
# RH SES lab hardware has some enclosures which don't work, which includes the ses tools from sg_utils
# and libStorageMgmt, so it's not something specific with ledmon project.
#
# To run this test do ./exercise_led_ctl ../src/ledctl sg3- sg2-
# to filter out enclosures that don't work
#
# exit code == 0 -> success


class Slot:
    def __init__(self, cntrl, slot_id, state, device_node):
        self.cntrl = cntrl
        self.slot = slot_id
        self.state = state.lower()
        self.device_node = None
        if device_node.startswith('/dev/'):
            self.device_node = device_node

    def __str__(self):
        return "slot: %s state: %s device: %s" % (self.slot, self.state, self.device_node)


def get_controllers_with_slot_functionality():
    rc = {}
    result = subprocess.run([LEDCTL_BIN, "--list-controllers"], capture_output=True)
    if result.returncode == 0:
        out = result.stdout.decode("utf-8")
        for raw_line in out.split("\n"):
            line = raw_line.strip()
            if "SCSI" in line:
                rc["SCSI"] = True
            elif "VMD" in line:
                rc["VMD"] = True
            elif "NPEM" in line:
                rc["NPEM"] = True
    return rc.keys()


def process_slot_line(controller, rawline):
    line = rawline.strip()
    match = SLOT_LINE.match(line)
    if match is not None:
        slot = Slot(controller, match.group(1).strip(), match.group(2).strip(), match.group(3).strip())
        for slot_filter in SLOT_FILTERS:
            if slot.slot.startswith(slot_filter):
                # Filter out this slot
                return None
        return slot
    return None


def get_slot(slot_o):
    result = subprocess.run([LEDCTL_BIN,
                             "--get-slot",
                             "--controller-type", slot_o.cntrl,
                             "--slot", slot_o.slot], capture_output=True)
    if result.returncode == 0:
        out = result.stdout.decode("utf-8")
        slot = process_slot_line(slot_o.cntrl, out)
        return slot
    else:
        print("Failed to set slot: {result}")
        return None


def get_slots(controller_type):
    rc = []
    result = subprocess.run([LEDCTL_BIN,
                             "--list-slots",
                             "--controller-type", controller_type], capture_output=True)
    if result.returncode == 0:
        out = result.stdout.decode("utf-8")
        for l in out.split("\n"):
            s = process_slot_line(controller_type, l)
            if s is not None:
                rc.append(s)
    return rc


def set_slot_state(slot_o, state):
    result = subprocess.run([LEDCTL_BIN,
                             "--set-slot",
                             "--controller-type", slot_o.cntrl,
                             "--slot", slot_o.slot,
                             "--state", state], capture_output=True)
    if result.returncode == 0:
        return True
    else:
        print("Failed to set slot: {result}")
        return False


def get_all_slots():
    all_slots = []
    for controller in get_controllers_with_slot_functionality():
        all_slots.extend(get_slots(controller))
    return all_slots


def set_state_by_dev_node(dev_node, state):
    option = "%s=%s" % (state, dev_node)
    result = subprocess.run([LEDCTL_BIN, option], capture_output=True)
    if result.returncode == 0:
        return True
    else:
        print(f"Error while using led non slot syntax {option} {result}")
        return False


def test_non_slot_set_path():
    """
    Test setting the led status by using non-slot syntax, eg. ledctl locate=/dev/sda
    :return:
    """
    slots_with_device_nodes = [s for s in get_all_slots() if s.device_node is not None]

    for slot in slots_with_device_nodes:
        for state in ["failure", "locate", "normal"]:
            set_state_by_dev_node(slot.device_node, state)
            cur = get_slot(slot)
            if cur.state != state:
                print(f"unable to set from {slot} to {state}, current = {cur} using non-slot syntax")
                sys.exit(1)


def test_slot_state_walk():
    """
    Test that we can set slots to different states and verify that they reported a change
    :return:
    """
    slots = get_all_slots()

    for slot in slots:
        for state in ["locate", "failure", "normal"]:
            result = set_slot_state(slot, state)
            if not result:
                print("failed to set slot state")
                sys.exit(1)
            cur = get_slot(slot)
            if cur.state != state:
                print(f"unable to set from {slot} to {state}, current = {cur}")
                sys.exit(1)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"syntax: {sys.argv[0]} <ledctl binary>")
        sys.exit(1)
    elif len(sys.argv) > 2:
        # optional slots to skip as known to be not working
        SLOT_FILTERS = sys.argv[2:]
        print(f"slot filter = {SLOT_FILTERS}")

    LEDCTL_BIN = sys.argv[1]
    test_non_slot_set_path()
    test_slot_state_walk()
    sys.exit(0)

