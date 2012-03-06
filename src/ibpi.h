/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ex: set tabstop=2 softtabstop=2 shiftwidth=2 expandtab: */

/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _IBPI_H_INCLUDED_
#define _IBPI_H_INCLUDED_

/**
 * @brief IBPI pattern identifies.
 *
 * The IBPI specification lists the following pattern names:
 *
 * - NORMAL      - either drive is present or missing, activity LED does not
 *                 matter. The rest of the LEDs are off.
 * - FAIL        - a block device has failed or is missing. Failure LED is active
 *                 and the behavior is depended on implementation of enclosure
 *                 management processor.
 * - REBUILD(_P) - this means a RAID device is recovering or rebuilding its data.
 *                 Depending on implementation of enclosure management processor
 *                 appropriate LED is blinking or solid.
 * - ICA         - In a Critical Array, this means a RAID device is degraded and
 *                 there's no spare device available.
 * - IFA         - In a Failed Array, this means a RAID device is damaged and
 *                 cannot be recovered or rebuild.
 * - PFA         - Predict Failure Analysis state means that a block device will
 *                 fail soon, so it must be replaced with working one.
 * - LOCATE      - turns Locate LED on to identify a block device or slot.
 *
 * Additionally the following patterns has been introduced, just for the purpose
 * of LED control utility.
 *
 * - UNKNOWN     - unknown IBPI pattern and it means do not control LEDs for
 *                 a device it is set (no LED management).
 * - ONESHOT_NORMAL - this state means that ledmon just started and it does not
 *                 know anything about existing patterns set, so it will off all
 *                 the LEDs just in case of any problem in the future. The state
 *                 is set, when a RAID device disappears, too. Oneshot means
 *                 as soon application applies the state it will change to UNKNOWN.
 */
enum ibpi_pattern {
  IBPI_PATTERN_UNKNOWN = 0,
  IBPI_PATTERN_NORMAL,
  IBPI_PATTERN_ONESHOT_NORMAL,
  IBPI_PATTERN_DEGRADED,
  IBPI_PATTERN_REBUILD,
  IBPI_PATTERN_REBUILD_P,
  IBPI_PATTERN_FAILED_ARRAY,
  IBPI_PATTERN_HOTSPARE,
  IBPI_PATTERN_PFA,
  IBPI_PATTERN_FAILED_DRIVE,
  IBPI_PATTERN_LOCATE,
  IBPI_PATTERN_LOCATE_OFF
};

extern const char *ibpi_str[];

#endif /* _IBPI_H_INCLUDED_ */
