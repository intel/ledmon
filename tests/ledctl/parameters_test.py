# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Intel Corporation.

from ledctl.ledctl_cmd import LedctlCmd
import pytest
import logging

LOGGER = logging.getLogger(__name__)

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
    "--ibpi --help",
    "--ibpi -T",
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
    "--ibpi normal=/dev/nvme0n1 -T",
    "--ibpi locate=/dev/sda,/dev/sdb,/dev/sdc -T",
    "--ibpi normal=/dev/nvme0n1 --listed-only -T",
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

@pytest.mark.parametrize("log_path_commands", [
    "--list-controllers --log=/root/test.log -T",
    "normal=/dev/nvme0n1 --listed-only -l /root/test.log -T",
],)
def test_parameters_log_path(ledctl_binary, log_path_commands):
    cmd = LedctlCmd(ledctl_binary)
    cmd.is_test_flag_enabled()
    output = cmd.run_ledctl_cmd_decode(log_path_commands.split())
    assert "LOG_PATH=/root/test.log" in output

def test_parameter_log_level_all_values(ledctl_binary):
    cmd = LedctlCmd(ledctl_binary)
    cmd.is_test_flag_enabled()
    tested_log_levels = ["warning", "debug", "all", "info", "quiet", "error"]
    for level in tested_log_levels:
        args = "--list-controllers -T --log-level=" + level
        output = cmd.run_ledctl_cmd_decode(args.split())
        assert "LOG_LEVEL=" + level.upper() in output
        args = "--list-controllers -T --" + level
        output = cmd.run_ledctl_cmd_decode(args.split())
        assert "LOG_LEVEL=" + level.upper() in output

FIRST_LINE_HELP = b"Intel(R) Enclosure LED Control Application"
LAST_LINE_HELP = b"Bugs should be reported at: https://github.com/intel/ledmon/issues\n"

def test_main_help(ledctl_binary):
    cmd = LedctlCmd(ledctl_binary)
    cmd.is_test_flag_enabled()
    res = cmd.run_ledctl_cmd("--help".split(), True)

    LOGGER.debug(f"Command returned:\n {res}")
    assert res.stdout.startswith(FIRST_LINE_HELP) and\
        res.stdout.endswith(LAST_LINE_HELP) and\
        res.stdout.find(b"Usage: ledctl --<mode> [option...] ...") != -1

@pytest.mark.parametrize("valid_mode_help_commands", [
    "--set-slot",
    "--get-slot",
    "--list-controllers",
    "--ibpi",
    "--list-slots"
],)
def test_mode_help(ledctl_binary, valid_mode_help_commands):
    cmd = LedctlCmd(ledctl_binary)
    cmd.is_test_flag_enabled()
    mode_help = str(valid_mode_help_commands) + " --help"
    res = cmd.run_ledctl_cmd(mode_help.split(), True)
    expected_usage_line = "Usage: ledctl " + valid_mode_help_commands + " [option...] ..."

    LOGGER.debug(f"Command returned:\n {res}")
    assert res.stdout.startswith(FIRST_LINE_HELP) and\
        res.stdout.endswith(LAST_LINE_HELP) and\
        res.stdout.decode().find(expected_usage_line) != -1

@pytest.mark.parametrize("valid_mode_help_commands_without_options", [
    "--version",
    "--help"
],)
def test_mode_help_without_options(ledctl_binary, valid_mode_help_commands_without_options):
    cmd = LedctlCmd(ledctl_binary)
    cmd.is_test_flag_enabled()
    mode_help = str(valid_mode_help_commands_without_options) + " --help"
    res = cmd.run_ledctl_cmd(mode_help.split(), True)
    expected_usage_line = "Usage: ledctl " + valid_mode_help_commands_without_options

    LOGGER.debug(f"Command returned:\n {res}")
    assert res.stdout.startswith(FIRST_LINE_HELP) and\
        res.stdout.endswith(LAST_LINE_HELP) and\
        res.stdout.decode().find(expected_usage_line) != -1
