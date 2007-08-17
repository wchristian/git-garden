# -*- Makefile -*-

AM_CFLAGS          = $(AC_CFLAGS)

supp_PROGRAMS      = bsvplay testdl pipe2pt qplay testdl utmp_register
supp_SCRIPTS       = diff2php extract_d3hog extract_d3pkg extract_f3pod \
                     extract_q2pak filenameconv findcont flv2avi fnt2bdf \
                     logontime mailsplit mod2ogg mpg2ogg netload png2wx.pl \
                     png2wx.py proc_stat_signal_decode raregetty \
                     sourcefuncsize vcsaview

bsvplay_SOURCES       = bsvplay.c pcspkr_pcm.c
bsvplay_LDADD         = -lHX -lm
qplay_SOURCES         = qplay.c pcspkr_pcm.c
qplay_LDADD           = -lHX -lm
testdl_LDFLAGS        = -lHX
utmp_register_LDFLAGS = -lHX
