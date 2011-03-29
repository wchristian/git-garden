#!/bin/bash

. /etc/hxloginpref.conf >/dev/null 2>/dev/null || :;

if [ "$HXPREF_ENABLE" == "yes" ]; then

hxtools_profile_setsbin ()
{
	local IFS=":";
	local path;
	local sbin;

	set -- $PATH;
	PATH="";

	for path; do
		sbin="";
		if [[ ! -d "$path" ]]; then
			continue;
		fi;
		PATH="$PATH:$path";
		if [[ "$path" == "/bin" || "$path" == "/usr/bin" ||
		      "$path" == "/usr/local/bin" ]]; then
			path="${path/bin/sbin}";
			if [[ -d "$path" ]]; then
				PATH="$PATH:$path";
			fi;
		fi;
	done;
	PATH="${PATH:1}";
}

hxtools_profile_main ()
{
	local isroot=0;
	[[ "$UID" -eq 0 ]] && isroot=1;

	export CVS_RSH="ssh";
	[[ -n "$EDITOR" ]] && export EDITOR;

	if [[ "$HXPREF_SBIN" == yes ]]; then
		hxtools_profile_setsbin;
	fi;

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
}

hxtools_profile_main;
unset hxtools_profile_main;
unset hxtools_profile_setsbin;

fi; # if [ "$HXPREF_ENABLE" == "yes" ];
