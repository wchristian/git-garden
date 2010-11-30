# -*- Makefile -*-

AM_CFLAGS        = ${regular_CFLAGS} ${libHX_CFLAGS}
CLEANFILES       = omixer orec

bin_PROGRAMS     = declone htruncate kps newns ofl oplay tailhex
bin_SCRIPTS      = checkbrack cwdiff fduphl \
                   fxterm git-author-stat git-export-patch \
                   git-forest git-lemon git-track gpsh man2html ln2 \
                   omixer orec pesubst pmap_dirty qtar recursive_lower \
                   su1 sysinfo wktimer
kps_LDADD        = ${libHX_LIBS}
ofl_LDADD        = ${libHX_LIBS}
oplay_LDADD      = ${libHX_LIBS}
tailhex_LDADD    = ${libHX_LIBS}

omixer${EXEEXT}: oplay${EXEEXT}
	if [ ! -e omixer ]; then ln -s oplay omixer; fi;

orec${EXEEXT}: oplay${EXEEXT}
	if [ ! -e orec ]; then ln -s oplay orec; fi;
