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

import pytest
import subprocess

class TestParameters():
        ledctl_bin = ""
        SUCCESS_EXIT_CODE = 0
        CMDLINE_ERROR_EXIT_CODE = 35

        def verify_if_flag_is_enabled(self):
                cmd = ("sudo " + self.ledctl_bin + " -T").split()
                result = subprocess.run(cmd)
                assert result.returncode == self.SUCCESS_EXIT_CODE,\
                        "Test flag is disabled. Please add configure option \"--enable-test\"."

        def run_ledctl_command(self, parameters):
                cmd = ("sudo " + self.ledctl_bin + " " + parameters).split()
                return subprocess.run(cmd)

        def test_parameters_are_valid_long_test_flag(self, ledctl_binary):
                self.ledctl_bin = ledctl_binary
                self.verify_if_flag_is_enabled()
                result = self.run_ledctl_command("--list-controllers --test")
                assert result.returncode == self.SUCCESS_EXIT_CODE


        @pytest.mark.parametrize("valid_mode_commands", [
                "-h",
                "--help",
                "-v",
                "--version",
                "-L -T",
                "--list-controllers -T",
                "-P -c vmd -T",
                "--list-slots --controller-type=vmd -T",
                "-G -c vmd -d /dev/nvme0n1 -T",
                "--get-slot --controller-type=vmd --device=/dev/nvme0n1 -T",
                "-G -c vmd -p 1 -T",
                "--get-slot --controller-type=vmd --slot=1 -T",
                "-S -c vmd -p 1 -s normal -T",
                "--set-slot --controller-type=vmd --slot=1 --state=normal -T"
        ],)
        def test_parameters_are_valid_short_test_flag(self, ledctl_binary, valid_mode_commands):
                self.ledctl_bin = ledctl_binary
                self.verify_if_flag_is_enabled()
                result = self.run_ledctl_command(valid_mode_commands)
                assert result.returncode == self.SUCCESS_EXIT_CODE


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
        def test_ibpi_parameters_are_valid_short_test_flag(self, ledctl_binary, valid_ibpi_commands):
                self.ledctl_bin = ledctl_binary
                self.verify_if_flag_is_enabled()
                result = self.run_ledctl_command(valid_ibpi_commands)
                assert result.returncode == self.SUCCESS_EXIT_CODE
