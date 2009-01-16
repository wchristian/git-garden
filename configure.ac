
AC_INIT([hxtools], [20090116])
AC_PREFIX_DEFAULT([/usr/local/hxtools])
AC_CONFIG_HEADERS([config.h])
AC_PROG_INSTALL
AM_INIT_AUTOMAKE([-Wall])
AC_PROG_CC
AM_PROG_CC_C_O

AC_CHECK_HEADERS([lastlog.h paths.h])
PKG_CHECK_MODULES([libHX], [libHX >= 2.2])
AC_CHECK_LIB([cap], [cap_get_flag], [have_libcap="yes"])
AM_CONDITIONAL([HAVE_LIBCAP], [test "$have_libcap" = "yes"])

kbddir='${prefix}/kbd';
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
	Most likely /usr/share/fonts.]),
	[x11fontdir="$withval"])
AC_SUBST([kbddir])
AC_SUBST([keymapdir])
AC_SUBST([vgafontdir])
AC_SUBST([x11fontdir])

regular_CFLAGS="-D_LARGEFILE_SOURCE=1 -D_LARGE_FILES -D_FILE_OFFSET_BITS=64 \
	-D_REENTRANT -Wall -Waggregate-return -Wmissing-declarations \
	-Wmissing-prototypes -Wredundant-decls -Wshadow -Wstrict-prototypes \
	-Winline -pipe"
AC_SUBST([regular_CFLAGS])
AC_CONFIG_FILES([Makefile bin/Makefile data/Makefile doc/Makefile
	kbd/GNUmakefile libexec/Makefile])
AC_OUTPUT
