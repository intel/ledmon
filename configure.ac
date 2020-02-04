
AC_PREREQ([2.69])
AC_INIT([ledmon], [0.94])
AC_CONFIG_MACRO_DIR([m4])
AC_SUBST([PACKAGE_DATE], "February 2020")
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AM_PROG_CC_C_O
AC_PROG_CC_C99
AC_PROG_INSTALL
AC_CONFIG_HEADERS([config_ac.h])

AM_CFLAGS='-Wall -I../config'
AM_CPPFLAGS='-D_DEBUG -D_GNU_SOURCE -D_DEFAULT_SOURCE -DDMALLOC_DISABLE -DBUILD_LABEL=\""$(BUILD_LABEL)"\"'


AC_DEFUN([AX_AM_CFLAGS_ADD],[AX_CHECK_COMPILE_FLAG($1, AM_CFLAGS="$AM_CFLAGS $1")])
AX_AM_CFLAGS_ADD([-Wformat -Werror=format-security])
AX_AM_CFLAGS_ADD([-Werror=format-overflow=2])
AX_AM_CFLAGS_ADD([-Werror=format-truncation=1])
AX_AM_CFLAGS_ADD([-Werror=shift-negative-value])
AX_AM_CFLAGS_ADD([-Werror=alloca])
AX_AM_CFLAGS_ADD([-Werror=missing-field-initializers])
AX_AM_CFLAGS_ADD([-Werror=format-signedness])

AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CPPFLAGS])

# Automake 1.11 - silent build rules
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

PKG_PROG_PKG_CONFIG

# Checks for libraries.
AC_CHECK_LIB([rt], [shm_unlink], [], [AC_MSG_ERROR(librt not found)])
AC_CHECK_LIB([sgutils2], [sg_ll_send_diag], [], [AC_MSG_ERROR(libsgutils not found)])
PKG_CHECK_MODULES([LIBUDEV], [libudev])
PKG_CHECK_MODULES([LIBPCI], [libpci])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h stdint.h stdlib.h string.h sys/file.h sys/ioctl.h sys/param.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([ftruncate memset munmap realpath regcomp select strcasecmp strchr strdup strerror strrchr strstr strtol strtoul])

# configure options
AC_ARG_ENABLE(systemd, AS_HELP_STRING([--enable-systemd], [install ledmon systemd service]))
AC_ARG_ENABLE(testconfig, AS_HELP_STRING([--enable-testconfig], [build test_config tool]))

AS_IF([test "x$enable_systemd" = xyes], [SYSTEMD_STR=yes], [SYSTEMD_STR=no])
AS_IF([test "x$enable_testconfig" = xyes], [TESTCONFIG_STR=yes], [TESTCONFIG_STR=no])

AM_CONDITIONAL([SYSTEMD_CONDITION], [test "$SYSTEMD_STR" = yes])
AM_CONDITIONAL([TESTCONFIG_CONDITION], [test "$TESTCONFIG_STR" = yes])

# target directory for ledmon service file
AC_SUBST([SYSTEMD_PATH], "$(pkg-config systemd --variable=systemdsystemunitdir)")

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 src/Makefile
                 systemd/Makefile])
AC_OUTPUT
AC_MSG_RESULT([
$PACKAGE_NAME $VERSION configuration:

  Source code location:    ${srcdir}
  Preprocessor flags:      ${AM_CPPFLAGS} ${CPPFLAGS}
  C compiler flags:        ${AM_CFLAGS} ${CFLAGS}
  Common install location: ${prefix}
  configure parameters:    --enable-systemd=${SYSTEMD_STR}
                           --enable-testconfig=${TESTCONFIG_STR}
])
