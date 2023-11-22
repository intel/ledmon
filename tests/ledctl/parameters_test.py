# Intel(R) Enclosure LED Utilities
# Copyright (C) 2009-2023 Intel Corporation.

# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.

# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.

# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

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
