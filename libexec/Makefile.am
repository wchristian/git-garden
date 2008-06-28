# -*- Makefile -*-

AM_CFLAGS     = $(regular_CFLAGS)

libexec_PROGRAMS = bsvplay fd0ssh graph-fanout graph-lchain longjumpcd \
	testdl pipepty qplay socketpty testdl utmp_register wavdiff xcp
if HAVE_LIBCAP
libexec_PROGRAMS += printcaps
endif

libexec_SCRIPTS  = diff2php doxygen-kerneldoc-filter extract_d3hog \
	extract_d3pkg extract_f3pod \
                extract_q2pak filenameconv flv2avi fnt2bdf git-revert-stats \
                logontime mailsplit mod2ogg mpg2ogg netload paddrspacesize \
                png2wx.pl png2wx.py proc_iomem_count \
                proc_stat_signal_decode raregetty \
                sourcefuncsize vcsaview

bsvplay_SOURCES     = bsvplay.c pcspkr_pcm.c
bsvplay_LDADD       = -lHX -lm
graph_fanout_LDADD  = -lHX
graph_lchain_LDADD  = -lHX
longjumpcd_LDADD    = -lHX
qplay_SOURCES       = qplay.c pcspkr_pcm.c
qplay_LDADD         = -lHX -lm
printcaps_LDADD     = -lcap
testdl_LDADD        = -lHX
utmp_register_LDADD = -lHX
wavdiff_LDADD       = -lHX
xcp_LDADD           = -lHX
