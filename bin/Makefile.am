# -*- Makefile -*-

AM_CFLAGS          = $(AC_CFLAGS)
CLEANFILES         = omixer orec

bin_PROGRAMS       = declone longjumpcd omixer oplay orec tailhex truncate
bin_SCRIPTS        = alplay checkbrack fxterm pesubst pmap_dirty qtar qtar2 \
                     recursive_lower rpmdep.pl stxdb svn-vup svntarsort \
                     sysinfo wktimer ytdl
longjumpcd_LDFLAGS = -lHX
oplay_LDFLAGS      = -lHX
tailhex_LDFLAGS    = -lHX

omixer: oplay
	[ ! -e omixer ] && ln -s oplay omixer;

orec: oplay
	[ ! -e orec ] && ln -s oplay orec;
