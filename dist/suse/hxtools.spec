
Name:		hxtools
Version:	20110308
Release:	0
Group:		System/Base
URL:		http://jengelh.medozas.de/projects/hxtools/
Summary:	Collection of day-to-day tools (binaries)

Source:		%name-%version.tar.xz
Source2:	%name-%version.tar.xz.asc
License:	GPL,PD
# freetype2, xorg-x11 for "bdftopcf"
BuildRequires:	libHX-devel >= 3.4, freetype2, libcap-devel
BuildRequires:	xorg-x11, pkg-config, xz
BuildRoot:	%_tmppath/%name-%version-build
Recommends:	%name-scripts = %version, %name-man = %version

%define build_profile 1

%description
A collection of various tools. Some of the important ones:

* declone(1) — break hardlinks
* fd0ssh(1) — pipe for password-over-stdin support to ssh
* newns(8) — clone current filesystem namespace and start a process
* ofl(1) — open file lister (replaces fuser and lsof -m)
* tailhex(1) — hex dumper with tail-following support
* utmp_register(1) — make entries in the utmp/wtmp database
* vfontas(1) — VGA font file assembler

%package scripts
Group:		System/Base
Summary:	Collection of day-to-day tools (scripts)
BuildArch:	noarch
Recommends:	%name, %name-man

%description scripts
Architecture-independent programs from hxtools.

* checkbrack(1) — check parenthesis and bracket count
* cwdiff(1) — run wdiff with color
* diff2php(1) — transform patch to self-serving PHP file
* doxygen-kerneldoc-filter(1) — filter for Doxygen to support kerneldoc
* filenameconv(1) — convert file name encoding
* flv2avi(1) — repackage Flash video into an AVI container with PCM audio
* fnt2bdf(1) — convert VGA raw fonts to X11 BDF
* git-author-stat(1) — show commit author statistics of a git repository
* git-export-patch(1) — produce perfect patch from git comits for mail submission
* git-forest(1) — display the commit history forest
* git-new-root(1) — start a new root in the git history
* git-revert-stats(1) — show reverting statistics of a git repository
* git-track(1) — set up branch for tracking a remote
* man2html(1) — convert nroff manpages to HTML
* pesubst(1) — perl-regexp stream substitution (replaces sed for sub‐ stitutions)
* pmap_dirty(1) — display amount of RAM a process uses hard
* recursive_lower(1) — recursively lowercase all filenames
* spec-beautifier(1) — program to clean up RPM .spec files
* sysinfo(1) — print IRC-style system information banner
* vcsaview(8) — display a screen dump in VCSA format
* wktimer(1) — work timer

%package man
Group:		Documentation/Man
Summary:	Manual pages for the hxtools suite
BuildArch:	noarch

%description man
This package contains the manual pages for the binaries and scripts
from hxtools.

%package data
Group:		System/Base
Summary:	Collection of day-to-day tools (data)
BuildArch:	noarch

%description data
Architecture-independent data from hxtools.

* VAIO U3 keymap
* additional fonts for console and xterm
* additional syntax highlighting definitions for mcedit

%package profile
Group:		System/Base
Summary:	The hxtools shell environment
Requires:	%name = %version, %name-scripts = %version
Requires:	%name-data = %version
BuildArch:	noarch

%description profile
Bash environment settings from hxtools. Particularly, this provides
the SUSE 6.x ls color scheme, and an uncluttered PS1 that shows
only important parts of a path.

%prep
%setup -q

%build
%configure \
	--datadir=%_datadir/%name \
	--with-keymapdir=%_datadir/kbd/keymaps \
	--with-vgafontdir=%_datadir/kbd/consolefonts \
	--with-x11fontdir=%_datadir/fonts
make %{?_smp_mflags};

%install
o="$PWD";
b="%buildroot";
rm -Rf "$b";
mkdir "$b";
make install DESTDIR="$b";

install -dm0755 "$b/%_datadir/mc/syntax";
install -pm0644 cooledit/*.syntax "$b/%_datadir/mc/syntax/";
install -dm0755 "$b/%_sysconfdir/openldap/schema";
install -pm0644 data/rfc2307bis-utf8.schema "$b/%_sysconfdir/openldap/schema/";

cd "$b";
find ./%_bindir ./%_libexecdir ! -type d -exec grep -l ELF {} + | \
	perl -pe 's{^\./+}{/}' >"$o/binary.lst";
find ./%_bindir ./%_libexecdir ! -type d -exec grep -L ELF {} + | \
	perl -pe 's{^\./+}{/}' >"$o/scripts.lst";

%if 0%{?build_profile}
mkdir -p "$b/%_sysconfdir/profile.d";
ln -s "%_datadir/%name/hxtools_profile.bash" "$b/%_sysconfdir/profile.d/z_hxtools_profile.sh";
%else
rm -Rf "$b/%_sysconfdir/profile.d" "$b/%_sysconfdir"/hx*;
%endif

%clean
rm -Rf "%buildroot";

%files -f binary.lst
%defattr(-,root,root)
%dir %_libexecdir/%name

%files scripts -f scripts.lst
%defattr(-,root,root)
%dir %_libexecdir/%name

%files man
%defattr(-,root,root)
%doc %_mandir/*/*

%files data
%defattr(-,root,root)
%dir %_sysconfdir/openldap
%dir %_sysconfdir/openldap/schema
%config %_sysconfdir/openldap/schema/*
%_datadir/%name
%_datadir/kbd
%_datadir/fonts/misc
%_datadir/mc

%if 0%{?build_profile}
%files profile
%defattr(-,root,root)
%config %_sysconfdir/hxloginpref.conf
%config %_sysconfdir/profile.d/*
%endif

%changelog
