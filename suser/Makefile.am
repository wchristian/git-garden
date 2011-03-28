# -*- Makefile -*-

AM_CPPFLAGS = ${regular_CPPFLAGS}
AM_CFLAGS   = ${regular_CFLAGS} ${libHX_CFLAGS}

bin_PROGRAMS = \
	declone \
	tailhex \
	xcp

bin_SCRIPTS = \
	fduphl \
	fxterm \
	pesubst \
	qtar \
	sysinfo \
	wktimer

pkglibexec_SCRIPTS = \
	mailsplit

tailhex_LDADD = ${libHX_LIBS}
xcp_LDADD     = ${libHX_LIBS}
