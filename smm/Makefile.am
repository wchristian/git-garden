# -*- Makefile -*-

AM_CPPFLAGS = ${regular_CPPFLAGS}
AM_CFLAGS   = ${regular_CFLAGS} ${libHX_CFLAGS}
CLEANFILES  = omixer orec

bin_PROGRAMS = \
	oplay

bin_SCRIPTS = \
	gpsh

pkglibexec_PROGRAMS = \
	bsvplay \
	qplay \
	vfontas \
	wavdiff

pkglibexec_SCRIPTS  = \
	extract_d3pkg \
	extract_dxhog \
	extract_f3pod \
	extract_qupak \
	fnt2bdf \
	mod2ogg \
	mpg2ogg \
	png2wx.pl \
	png2wx.py \
	shared.pm \
	stxdb

bsvplay_SOURCES = bsvplay.c pcspkr_pcm.c
bsvplay_LDADD   = ${libHX_LIBS} -lm
oplay_LDADD     = ${libHX_LIBS}
qplay_SOURCES   = qplay.c pcspkr_pcm.c
qplay_LDADD     = ${libHX_LIBS} -lm
wavdiff_LDADD   = ${libHX_LIBS}
vfontas_LDADD   = ${libHX_LIBS}

omixer${EXEEXT}: oplay${EXEEXT}
	if [ ! -e omixer ]; then ln -s oplay omixer; fi;

orec${EXEEXT}: oplay${EXEEXT}
	if [ ! -e orec ]; then ln -s oplay orec; fi;

all-local: omixer${EXEEXT} orec${EXEEXT}

install-exec-local:
	test -z "${bindir}" || ${MKDIR_P} "${DESTDIR}${bindir}";
	ln -s "oplay${EXEEXT}" "${DESTDIR}${bindir}/orec${EXEEXT}";
	ln -s "oplay${EXEEXT}" "${DESTDIR}${bindir}/omixer${EXEEXT}";
