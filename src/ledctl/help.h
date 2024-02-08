/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Intel Corporation.
 */

#ifndef HELP_H_
#define HELP_H_

#include "utils.h"

void print_mode_help(enum opt mode_id);
void _print_incorrect_help_usage(void);
void _print_main_help(void);
void _ledctl_version(void);

#endif // HELP_H_INCLUDED_
