/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2022-2023 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
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
 * - FAIL        - a block device has failed or is missing. Failure LED is
 *                 active and the behavior is depended on implementation
 *                 of enclosure management processor.
 * - REBUILD     - this means a RAID device is recovering or rebuilding
 *                 its data. Depending on implementation of enclosure
 *                 management processor appropriate LED is blinking or solid.
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
 *                 as soon application applies the state it will change
 *                 to UNKNOWN.
 * - ADDED         this state means that device previously known to ledmon is
 *                 restored. This state will be changed to ONESHOT_NORMAL.
 * - REMOVED       this state means that device was removed from system. It
 *                 will be changed to ADDED after restoring device to system.
 */
enum ibpi_pattern {
	IBPI_PATTERN_UNKNOWN = 0,
	IBPI_PATTERN_NONE,	/* used only to initialize ibpi_prev */
	IBPI_PATTERN_NORMAL,
	IBPI_PATTERN_ONESHOT_NORMAL,
	IBPI_PATTERN_DEGRADED,
	IBPI_PATTERN_HOTSPARE,
	IBPI_PATTERN_REBUILD,
	IBPI_PATTERN_FAILED_ARRAY,
	IBPI_PATTERN_PFA,
	IBPI_PATTERN_FAILED_DRIVE,
	IBPI_PATTERN_LOCATE,
	IBPI_PATTERN_LOCATE_OFF,
	IBPI_PATTERN_ADDED,
	IBPI_PATTERN_REMOVED,
	/* Below are SES-2 codes. Note that by default most IBPI messages are
	 * translated into SES when needed but SES codes can be added also. */
	SES_REQ_ABORT,
	SES_REQ_REBUILD,
	SES_REQ_IFA,
	SES_REQ_ICA,
	SES_REQ_CONS_CHECK,
	SES_REQ_HOSTSPARE,
	SES_REQ_RSVD_DEV,
	SES_REQ_OK,
	SES_REQ_IDENT,
	SES_REQ_RM,
	SES_REQ_INS,
	SES_REQ_MISSING,
	SES_REQ_DNR,
	SES_REQ_ACTIVE,
	SES_REQ_EN_BB,
	SES_REQ_EN_BA,
	SES_REQ_DEV_OFF,
	SES_REQ_FAULT,
	SES_REQ_PRDFAIL,
	ibpi_pattern_count,
};

extern const char *ibpi_str[ibpi_pattern_count];

#endif				/* _IBPI_H_INCLUDED_ */
