# -*- Makefile -*-

AM_CFLAGS        = $(AC_CFLAGS)
CLEANFILES       = omixer orec

bin_PROGRAMS     = declone longjumpcd omixer oplay orec tailhex truncate
bin_SCRIPTS      = checkbrack fxterm pesubst playmuch pmap_dirty qtar qtar2 \
                   recursive_lower rpmdep.pl stxdb svn-pa svn-vup svntarsort \
                   sysinfo wktimer ytdl
longjumpcd_LDADD = -lHX
oplay_LDADD      = -lHX
tailhex_LDADD    = -lHX

omixer: oplay
	[ ! -e omixer ] && ln -s oplay omixer;

orec: oplay
	[ ! -e orec ] && ln -s oplay orec;
