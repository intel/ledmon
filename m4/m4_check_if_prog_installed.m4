# SYNOPSIS
#
#   AX_CHECK_PROG
#
# DESCRIPTION
#
#   Check if the given program installed, let script continue if exists, pops up
#   error message if not.
#
#   Besides checking existence, this macro also set these environment
#   variables upon completion:
#
#     PROG_"prog_name" = result of checking if program installed (yes/no)

AC_DEFUN([AX_CHECK_PROG],
[dnl
  AC_CHECK_PROG([PROG_$1], [$1], [yes], [no])

  AS_IF([test "$PROG_$1" = "no"],
  [dnl
    AC_MSG_ERROR([Utility "$1" not found.])
  ])
])