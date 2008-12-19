# -*- Makefile -*-

AM_CFLAGS        = ${regular_CFLAGS} ${libHX_CFLAGS}
CLEANFILES       = omixer orec

bin_PROGRAMS     = declone newns ofl oplay tailhex truncate
bin_SCRIPTS      = checkbrack cwdiff fxterm git-author-stat git-export-patch \
                   git-forest git-lemon git-track man2html \
                   omixer orec pesubst playalot \
                   pmap_dirty qtar recursive_lower \
                   sysinfo wktimer
ofl_LDADD        = ${libHX_LIBS}
oplay_LDADD      = ${libHX_LIBS}
tailhex_LDADD    = ${libHX_LIBS}

omixer${EXEEXT}: oplay${EXEEXT}
	if [ ! -e omixer ]; then ln -s oplay omixer; fi;

orec${EXEEXT}: oplay${EXEEXT}
	if [ ! -e orec ]; then ln -s oplay orec; fi;
