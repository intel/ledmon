# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Red Hat Inc.

# Simple tests for ledctl.  This needs to be expanded.

import pytest
import sys
import subprocess
import re
import logging

LOGGER = logging.getLogger(__name__)

# Note:
# RH SES lab hardware has some enclosures which don't work, which includes the ses tools from sg_utils
# and libStorageMgmt, so it's not something specific with ledmon project.
#
# To run this test do `pytest tests --ledctl-binary=../src/ledctl --slot-filters=sg3-,sg2-`
# to filter out enclosures that don't work

class Slot:
    def __init__(self, cntrl_type, slot_id, state, device_node):
        self.cntrl_type = cntrl_type
        self.slot = slot_id
        self.state = state.lower()
        self.device_node = None
        if device_node.startswith('/dev/'):
            self.device_node = device_node

    def __str__(self):
        return "slot: %s state: %s device: %s" % (self.slot, self.state, self.device_node)

class TestSlot():

    ledctl_bin = ""

    def get_controllers_with_slot_functionality(self):
        rc = {}
        result = subprocess.run([self.ledctl_bin, "--list-controllers"], capture_output=True)
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


    def process_slot_line(self, controller, rawline, slot_filters):
        SLOT_LINE = re.compile(r"^slot: (.+)led state:(.+)device:(.+)$")
        line = rawline.strip()
        match = SLOT_LINE.match(line)
        if match is not None:
            slot = Slot(controller, match.group(1).strip(), match.group(2).strip(), match.group(3).strip())
            for slot_filter in slot_filters:
                if slot.slot.startswith(slot_filter):
                    # Filter out this slot
                    return None
            return slot
        return None


    def get_slot(self, slot_o, slot_filter):
        result = subprocess.run([self.ledctl_bin,
                                "--get-slot",
                                "--controller-type", slot_o.cntrl_type,
                                "--slot", slot_o.slot], capture_output=True)
        if result.returncode == 0:
            out = result.stdout.decode("utf-8")
            slot = self.process_slot_line(slot_o.cntrl_type, out, slot_filter)
            return slot
        else:
            LOGGER.error(f"Failed to set slot: {result}")
            return None


    def get_slots(self, controller_type, slot_filter):
        rc = []
        result = subprocess.run([self.ledctl_bin,
                                "--list-slots",
                                "--controller-type", controller_type], capture_output=True)
        if result.returncode == 0:
            out = result.stdout.decode("utf-8")
            for l in out.split("\n"):
                s = self.process_slot_line(controller_type, l, slot_filter)
                if s is not None:
                    rc.append(s)
        return rc


    def set_slot_state(self, slot_o, state):
        result = subprocess.run([self.ledctl_bin,
                                "--set-slot",
                                "--controller-type", slot_o.cntrl_type,
                                "--slot", slot_o.slot,
                                "--state", state], capture_output=True)
        if result.returncode == 0:
            return True
        else:
            LOGGER.error(f"Failed to set slot: {result}")
            return False


    def get_all_slots(self, slot_filter):
        all_slots = []
        for controller in self.get_controllers_with_slot_functionality():
            all_slots.extend(self.get_slots(controller, slot_filter))
        return all_slots


    def set_state_by_dev_node(self, dev_node, state):
        option = "%s=%s" % (state, dev_node)
        result = subprocess.run([self.ledctl_bin, option], capture_output=True)
        if result.returncode == 0:
            return True
        else:
            LOGGER.error(f"Error while using led non slot syntax {option} {result}")
            return False


    def test_non_slot_set_path(self, ledctl_binary, slot_filters):
        """
        Test setting the led status by using non-slot syntax, eg. ledctl locate=/dev/sda
        :return:
        """
        self.ledctl_bin = ledctl_binary
        slots_with_device_nodes = [s for s in self.get_all_slots(slot_filters) if s.device_node is not None]

        for slot in slots_with_device_nodes:
            for state in ["failure", "locate", "normal"]:
                self.set_state_by_dev_node(slot.device_node, state)
                cur = self.get_slot(slot, slot_filters)
                assert cur.state == state, f"unable to set from {slot} to {state}, current = {cur} using non-slot syntax"


    def test_slot_state_walk(self, ledctl_binary, slot_filters):
        """
        Test that we can set slots to different states and verify that they reported a change
        :return:
        """
        self.ledctl_bin = ledctl_binary
        slots = self.get_all_slots(slot_filters)

        for slot in slots:
            for state in ["locate", "failure", "normal"]:
                result = self.set_slot_state(slot, state)
                assert result == True, "failed to set slot state"
                cur = self.get_slot(slot, slot_filters)
                assert cur.state == state, f"unable to set from {slot} to {state}, current = {cur}"
