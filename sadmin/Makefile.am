# -*- Makefile -*-

AM_CPPFLAGS = ${regular_CPPFLAGS}
AM_CFLAGS   = ${regular_CFLAGS} ${libHX_CFLAGS}

bin_PROGRAMS = \
	newns \
	ofl

bin_SCRIPTS = \
	pmap_dirty \
	recursive_lower \
	su1

pkglibexec_PROGRAMS = \
	fd0ssh \
	printcaps \
	proc_iomem_count \
	psthreads \
	utmp_register \
	xfs_irecover

pkglibexec_SCRIPTS = \
	filenameconv \
	logontime \
	netload \
	raregetty \
	rpmdep.pl \
	vcsaview

ofl_LDADD              = ${libHX_LIBS}
printcaps_LDADD        = ${libcap_LIBS}
proc_iomem_count_LDADD = ${libHX_LIBS}
psthreads_LDADD        = ${libHX_LIBS}
utmp_register_LDADD    = ${libHX_LIBS}
xfs_irecover_LDADD     = ${libHX_LIBS}
