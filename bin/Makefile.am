# -*- Makefile -*-

AM_CFLAGS        = $(regular_CFLAGS)
CLEANFILES       = omixer orec

bin_PROGRAMS     = declone newns ofl omixer oplay orec tailhex truncate
bin_SCRIPTS      = checkbrack fxterm git-author-stat git-export-patch \
                   git-forest git-lemon man2html pesubst playalot \
                   pmap_dirty qtar recursive_lower rpmdep.pl stxdb \
                   sysinfo wktimer
ofl_LDADD        = -lHX
oplay_LDADD      = -lHX
tailhex_LDADD    = -lHX

omixer: oplay
	if [ ! -e omixer ]; then ln -s oplay omixer; fi;

orec: oplay
	if [ ! -e orec ]; then ln -s oplay orec; fi;
