# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Intel Corporation.

from ledctl.ledctl_cmd import LedctlCmd
import pytest

SUCCESS_EXIT_CODE = 0
CMDLINE_ERROR_EXIT_CODE = 35

def test_parameters_are_valid_long_test_flag(ledctl_binary):
    cmd = LedctlCmd(ledctl_binary)
    cmd.is_test_flag_enabled()
    cmd.run_ledctl_cmd("--list-controllers --test".split())

@pytest.mark.parametrize("valid_mode_commands", [
    "-h",
    "--help",
    "-v",
    "--version",
    "-L -T",
    "--list-controllers -T",
    "-P -n vmd -T",
    "--list-slots --controller-type=vmd -T",
    "-G -n vmd -d /dev/nvme0n1 -T",
    "--get-slot --controller-type=vmd --device=/dev/nvme0n1 -T",
    "-G -n vmd -p 1 -T",
    "--get-slot --controller-type=vmd --slot=1 -T",
    "-S -n vmd -p 1 -s normal -T",
    "--set-slot --controller-type=vmd --slot=1 --state=normal -T"
],)
def test_parameters_are_valid_short_test_flag(ledctl_binary, valid_mode_commands):
    cmd = LedctlCmd(ledctl_binary)
    cmd.is_test_flag_enabled()
    cmd.run_ledctl_cmd(valid_mode_commands.split())

@pytest.mark.parametrize("valid_ibpi_commands", [
    "normal=/dev/nvme0n1 -T",
    "normal=/dev/nvme0n1 -x -T",
    "normal=/dev/nvme0n1 --listed-only -T",
    "normal=/dev/nvme0n1 -l /var/log/ledctl.log -T",
    "normal=/dev/nvme0n1 --log=/var/log/ledctl.log -T",
    "normal=/dev/nvme0n1 --log-level=all -T",
    "normal=/dev/nvme0n1 --all -T",
    "-x normal={ /dev/nvme0n1 } -T",
    "locate=/dev/nvme0n1 rebuild={ /sys/block/sd[a-b] } -T",
    "off={ /dev/sda /dev/sdb } -T",
    "locate=/dev/sda,/dev/sdb,/dev/sdc -T"
],)
def test_ibpi_parameters_are_valid_short_test_flag(ledctl_binary, valid_ibpi_commands):
    cmd = LedctlCmd(ledctl_binary)
    cmd.is_test_flag_enabled()
    cmd.run_ledctl_cmd(valid_ibpi_commands.split())
