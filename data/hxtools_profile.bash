#!/bin/bash

. /etc/hxloginpref.conf >/dev/null 2>/dev/null || :;

if [ "$HXPREF_ENABLE" == "yes" ]; then

# --- main big block ---

isroot=0;
[[ "$UID" -eq 0 ]] && isroot=1;

export CVS_RSH="ssh";
[[ -n "$EDITOR" ]] && export EDITOR;

if [[ "$HXPREF_COLORS" == yes ]]; then
	#
	# See if grep knows about --color, and set inverse-cyan
	#
	grep --color=auto x /dev/null &>/dev/null;
	[[ "$?" -ne 2 ]] && alias grep="command grep --color=auto";
	pcregrep --color=auto x /dev/null &>/dev/null;
	[[ "$?" -ne 2 ]] && alias pcregrep="command pcregrep --color=auto";
	export GREP_COLOR="36;7";
	export PCREGREP_COLOR="36;7";
fi;

#
# Retain language across sudo and `bash --login` (which would otherwise
# re-source /etc/sysconfig/language)
#
if [[ -n "$HX_LC_MESSAGES" ]]; then
	export LC_MESSAGES="$HX_LC_MESSAGES";
else
	export HX_LC_MESSAGES="$LC_MESSAGES";
fi;

# --- end big main block ---

fi; # if [ "$HXPREF_ENABLE" == "yes" ];
