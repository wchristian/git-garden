# -*- Makefile -*-

AM_CFLAGS = ${regular_CFLAGS} ${libHX_CFLAGS}

libexec_PROGRAMS = bsvplay fd0ssh graph-fanout graph-lchain longjumpcd \
	testdl qplay testdl utmp_register wavdiff vfontas xcp
if HAVE_LIBCAP
libexec_PROGRAMS += printcaps
endif

libexec_SCRIPTS  = diff2php doxygen-kerneldoc-filter extract_dxhog \
	extract_d3pkg extract_f3pod \
                extract_qupak filenameconv flv2avi fnt2bdf git-revert-stats \
                logontime mailsplit mod2ogg mpg2ogg netload paddrspacesize \
                png2wx.pl png2wx.py proc_iomem_count \
                proc_stat_signal_decode raregetty \
                sourcefuncsize vcsaview

bsvplay_SOURCES     = bsvplay.c pcspkr_pcm.c
bsvplay_LDADD       = ${libHX_LIBS} -lm
graph_fanout_LDADD  = ${libHX_LIBS}
graph_lchain_LDADD  = ${libHX_LIBS}
longjumpcd_LDADD    = ${libHX_LIBS}
qplay_SOURCES       = qplay.c pcspkr_pcm.c
qplay_LDADD         = ${libHX_LIBS} -lm
printcaps_LDADD     = -lcap
testdl_LDADD        = ${libHX_LIBS}
utmp_register_LDADD = ${libHX_LIBS}
wavdiff_LDADD       = ${libHX_LIBS}
vfontas_LDADD       = ${libHX_LIBS}
xcp_LDADD           = ${libHX_LIBS}
