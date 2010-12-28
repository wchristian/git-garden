# -*- Makefile -*-

AM_CPPFLAGS = ${regular_CPPFLAGS}
AM_CFLAGS = ${regular_CFLAGS} ${libHX_CFLAGS}

pkglibexec_PROGRAMS = bsvplay cctypeinfo fd0ssh graph-fanout \
	graph-lchain proc_iomem_count \
	qplay testdl utmp_register wavdiff vfontas xcp \
	xfs_irecover
if HAVE_LIBCAP
pkglibexec_PROGRAMS += printcaps
endif

pkglibexec_SCRIPTS = diff2php doxygen-kerneldoc-filter extract_dxhog \
	extract_d3pkg extract_f3pod \
                extract_qupak filenameconv flv2avi flv2mka fnt2bdf \
                git-blame-stats git-new-root git-revert-stats \
                logontime mailsplit mod2ogg mpg2ogg netload paddrspacesize \
                png2wx.pl \
                proc_stat_signal_decode raregetty rpmdep.pl \
                sourcefuncsize stxdb vcsaview
noinst_SCRIPTS = png2wx.py

bsvplay_SOURCES     = bsvplay.c pcspkr_pcm.c
bsvplay_LDADD       = ${libHX_LIBS} -lm
graph_fanout_LDADD  = ${libHX_LIBS}
graph_lchain_LDADD  = ${libHX_LIBS}
qplay_SOURCES       = qplay.c pcspkr_pcm.c
qplay_LDADD         = ${libHX_LIBS} -lm
printcaps_LDADD     = ${libcap_LIBS}
proc_iomem_count_LDADD = ${libHX_LIBS}
testdl_LDADD        = ${libHX_LIBS} ${libdl_LIBS}
utmp_register_LDADD = ${libHX_LIBS}
wavdiff_LDADD       = ${libHX_LIBS}
vfontas_LDADD       = ${libHX_LIBS}
xcp_LDADD           = ${libHX_LIBS}
xfs_irecover_LDADD  = ${libHX_LIBS}
