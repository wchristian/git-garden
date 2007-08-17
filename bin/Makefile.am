# -*- Makefile -*-

AM_CFLAGS          = $(AC_CFLAGS)
CLEANFILES         = omixer orec

bin_PROGRAMS       = declone longjumpcd omixer oplay orec tailhex truncate
bin_SCRIPTS        = checkbrack fxterm pesubst pmap_dirty recursive_lower \
                     rpmdep.pl svn-vup svntarsort sysinfo wktimer ytdl
longjumpcd_LDFLAGS = -lHX
oplay_LDFLAGS      = -lHX
tailhex_LDFLAGS    = -lHX

omixer: oplay
	ln -s oplay omixer;

orec: oplay
	ln -s oplay orec;
