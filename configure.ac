
AC_INIT(hxtools, SVN)
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE
AC_PROG_CC

AC_CHECK_HEADER([libHX.h], [],
	AC_MSG_RESULT([Some programs need libHX 1.10 (or newer) but which is not installed. Use `make -k` to ignore build errors on these programs.]), [
#include <libHX.h>
#if !defined(_LIBHX_H) || _LIBHX_H < 20070701
#	error libHX not found or too old
#endif
])

kbddir='${prefix}/kbd';
suppdir='${exec_prefix}/supp';
keymapdir='${kbddir}';
vgafontdir='${kbddir}';
x11fontdir='${kbddir}';
AC_ARG_WITH([keymapdir], AS_HELP_STRING([--with-keymapdir=PATH],
	[Path to the keymap directory [[PREFIX/kbd]].
	Most likely /usr/share/kbd/keymaps.]),
	[keymapdir="$withval"])
AC_ARG_WITH([vgafontdir], AS_HELP_STRING([--with-vgafontdir=PATH],
	[Path to the VGA font directory [[PREFIX/kbd]].
	Most likely /usr/share/kbd/consolefonts.]),
	[vgafontdir="$withval"])
AC_ARG_WITH([x11fontdir], AS_HELP_STRING([--with-x11fontdir=PATH],
	[Path to the X11 font directory [[PREFIX/kbd]].
	Most likely /usr/share/fonts/misc.]),
	[x11fontdir="$withval"])
AC_SUBST(suppdir)
AC_SUBST(keymapdir)
AC_SUBST(vgafontdir)
AC_SUBST(x11fontdir)

AC_CFLAGS="-D_LARGEFILE_SOURCE=1 -D_LARGE_FILES -D_FILE_OFFSET_BITS=64 \
	-D_REENTRANT -Wall -Waggregate-return -Wmissing-declarations \
	-Wmissing-prototypes -Wredundant-decls -Wshadow -Wstrict-prototypes \
	-Winline -pipe"
AC_SUBST(AC_CFLAGS)
AC_OUTPUT(Makefile bin/Makefile kbd/GNUmakefile supp/Makefile)
