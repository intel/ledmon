#ifndef _IBPI_LOGGING_H_INCLUDED_
#define _IBPI_LOGGING_H_INCLUDED_

/**
 * @brief IBPI pattern names.
 *
 * This is internal array holding names of IBPI pattern. Logging routines use
 * this entries to translate enumeration type values into the string.
 */
const char *ibpi_str[] = {
	[IBPI_PATTERN_UNKNOWN]	      = "",
	[IBPI_PATTERN_NORMAL]	      = "NORMAL",
	[IBPI_PATTERN_ONESHOT_NORMAL] = "",
	[IBPI_PATTERN_DEGRADED]	      = "ICA",
	[IBPI_PATTERN_REBUILD]	      = "REBUILD",
	[IBPI_PATTERN_FAILED_ARRAY]   = "IFA",
	[IBPI_PATTERN_HOTSPARE]	      = "HOTSPARE",
	[IBPI_PATTERN_PFA]	      = "PFA",
	[IBPI_PATTERN_FAILED_DRIVE]   = "FAILURE",
	[IBPI_PATTERN_LOCATE]	      = "LOCATE",
	[IBPI_PATTERN_LOCATE_OFF]     = "LOCATE_OFF",
	[IBPI_PATTERN_ADDED]	      = "ADDED",
	[IBPI_PATTERN_REMOVED]	      = "REMOVED"
};

#endif			/* _IBPI_LOGGING_H_INCLUDED_ */
