
Name:		hxtools
Version:	20081224
Release:	0
Group:		System/Base
URL:		http://jengelh.medozas.de/projects/hxtools/
Summary:	Collection of day-to-day tools

Source:		%name-%version.tar.bz2
License:	GPL,PD
# freetype2, xorg-x11 for "bdftopcf"
BuildRequires:	libHX-devel >= 2.0, freetype2, libcap-devel
BuildRequires:	xorg-x11, pkg-config
BuildRoot:	%_tmppath/%name-%version-build
Recommends:	hxtools-noarch = %version
Prefix:		/opt/hxtools

%description
A big tool collection.

%package noarch
Group:		System/Base
Summary:	Collection day-to-day tools (data)
Requires:	hxtools = %version
Obsoletes:	hxtools-data

%description noarch
Architecture-indepent data for hxtools.

%debug_package
%prep
%setup

%build
%configure \
	--prefix=%prefix \
	--exec-prefix=%prefix \
	--bindir=%prefix/bin \
	--libexecdir=%prefix/libexec \
	--with-keymapdir=%_datadir/kbd/keymaps \
	--with-vgafontdir=%_datadir/kbd/consolefonts \
	--with-x11fontdir=%_datadir/fonts
make %{?jobs:-j%jobs};

%install
o="$PWD";
b="%buildroot";
rm -Rf "$b";
mkdir "$b";
make install DESTDIR="$b";
install -dm0755 "$b/%_sysconfdir/openldap/schema" "$b/%_datadir/mc/syntax";
install -pm0644 cooledit/*.syntax "$b/%_datadir/mc/syntax/";
install -pm0644 data/rfc2307bis-utf8.schema "$b/%_sysconfdir/openldap/schema/";
cd "$b";
find opt/hxtools -type f -print0 | \
	xargs -0 grep -l ELF | perl -ne 'print"/$_"' >"$o/binary.lst";
find opt/hxtools -type f -print0 | \
	xargs -0 grep -L ELF | perl -ne 'print"/$_"' >"$o/noarch.lst";

%clean
rm -Rf "%buildroot";

%files -f binary.lst
%defattr(-,root,root)

%files noarch -f noarch.lst
%defattr(-,root,root)
%dir %prefix
%_sysconfdir/hx*
%_sysconfdir/openldap/schema/*
%_datadir/kbd/consolefonts/*
%_datadir/kbd/keymaps/i386/*/*
%_datadir/fonts/misc/*
%_datadir/mc/syntax/*
%doc %_mandir/*/*
