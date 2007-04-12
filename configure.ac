
AC_INIT(hxtools, SVN)
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE
AC_PROG_CC

AC_CHECK_HEADER([libHX.h], [],
	AC_MSG_RESULT([Some programs need libHX 1.10 (or newer) but which is not installed. Use `make -k` to ignore build errors on these programs.]), [
#include <libHX.h>
#if !defined(_LIBHX_H) || _LIBHX_H < 20070320
#	error libHX not found or too old
#endif
])

AC_CFLAGS="-D_LARGEFILE_SOURCE=1 -D_LARGE_FILES -D_FILE_OFFSET_BITS=64 \
	-D_REENTRANT -Wall -Waggregate-return -Wmissing-declarations \
	-Wmissing-prototypes -Wredundant-decls -Wshadow -Wstrict-prototypes \
	-Winline -pipe"
AC_SUBST(AC_CFLAGS)
AC_OUTPUT(Makefile bin/Makefile kbd/GNUmakefile)
