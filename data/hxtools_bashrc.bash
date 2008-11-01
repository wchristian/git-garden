#!/bin/bash

. /etc/hxloginpref.conf &>/dev/null || :;

if [ "$AAA_HXTOOLS" == "yes" ]; then

# --- main big block ---

isroot=0;
[ "$UID" -eq 0 ] && isroot=1;

function extended_pushd()
{
	local path;
	if [ $# -eq 0 ]; then
		pushd .;
	else
		for path in "$@"; do
			pushd "$path";
		done;
	fi;
}

function beautify_path()
{
	# Show at most the last two path components, if the whole depth of a
	# normalized $PATH is more than 3 levels, e.g.
	
	#
	#   PATH                                    (NORMALIZED DEPTH LEVEL)
	#
	#   /J/kernel/linux-2.6.17.6                (3)
	#           -> /D/kernel/linux-2.6.17.6     (3)
	#   /J/kernel/linux-2.6.17.6/fs             (4)
	#           -> ../linux-2.6.17.6/fs         (3)
	#   ~/Coding/hxtools                        (3)
	#           -> ~/Coding/hxtools             (3)
	#   ~/Coding/hxtools/bin                    (4)
	#           -> ../hxtools/bin               (3)
	#
	local pw2;
	local wd;
	(
	IFS="/";
	pw2="$PWD/";
	# Special treatment for homedir, as ~ is cheaper (shorter) than
	# the full home path ($HOME)
	wd=${pw2/#$HOME\//~\/};
	wd=${wd%/};
	set $wd;
	if [ \( -z "$1" -a $# -le "$[$BEAUTIFY_PATH_LEVEL+1]" \) -o $# -le \
	    "$BEAUTIFY_PATH_LEVEL" ]; then
		echo "$wd";
	else
		eval "echo -en \"../\${$[$#-1]}/\${$#}\"";
	fi;
	);
}

function xd()
{
	local result;
	if [ -z "$1" ]; then
		builtin cd -;
		return $?;
	fi;
	result="`longjumpcd \"$1\" \"$2\"`";
	if [ -z "$result" ]; then
		echo -e "longjumpcd: no suitable path found";
		return 2;
	fi;
	builtin cd "$result";
	return $?;
}

# Allow truncation via > operator
set +C;

export PS1_ROOT="\A \h:\$(beautify_path) # ";
export PS1_USER="\A \h:\$(beautify_path) > ";
export PS1_XROOT="\[\e[1;30m\]\A \[\e[0;31m\]\h:\$(beautify_path) \[\e[1m\]#\[\e[0m\] ";
export PS1_XUSER="\[\e[1;30m\]\A \[\e[0;32m\]\h:\$(beautify_path) \[\e[1;37m\]>\[\e[0m\] ";

# Find out what options this machine's "ls" supports
# Same option finding for "less"
#
if [ "`uname -s`" == Linux ]; then
	LESS="-Mi";
	LS_OPTIONS="-NT0";
	for i in --color=auto --group-dir; do
		/bin/ls "$i" -d >/dev/null 2>&1 && LS_OPTIONS="$LS_OPTIONS $i";
	done;
else
	unset LESS;
	unset LS_OPTIONS;
	for i in -M -i; do
		less "$i" </dev/null >/dev/null 2>&1 && LESS="$LESS $i";
	done;
	for i in --color=auto --group-dir -N "-T 0"; do
		/bin/ls "$i" -d >/dev/null 2>&1 && LS_OPTIONS="$LS_OPTIONS $i";
	done;
fi;

export LESS;
export LS_OPTIONS;

if [ "$COLORS" == yes ]; then
	if [ "$isroot" -eq 1 ]; then
		export PS1="$PS1_XROOT";
	else
		export PS1="$PS1_XUSER";
	fi;
else
	unset LS_COLORS;
	if [ "$isroot" -eq 1 ]; then
		export PS1="$PS1_ROOT";
	else
		export PS1="$PS1_USER";
	fi;
fi;

unalias cd.. dir la ll ls-l you 2>/dev/null;
alias -- +="extended_pushd";
alias -- -="popd";
alias ..="cd ../";
alias ...="cd ../../";
alias cp="cp -ip";
alias e='$EDITOR';
alias ls='command ls $LS_OPTIONS';
alias l='command ls $LS_OPTIONS -al';
alias md="mkdir -p";
alias mv="mv -i";
alias o="less -S";
alias rd="rmdir";
alias rf="command rm";
alias rm="command rm -i";

# --- end big main block ---

fi; # if [ "$AAA_HXTOOLS" == "yes" ];
