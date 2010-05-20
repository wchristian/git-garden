
Name:		hxtools
Version:	20100520
Release:	0
Group:		System/Base
URL:		http://jengelh.medozas.de/projects/hxtools/
Summary:	Collection of day-to-day tools

Source:		%name-%version.tar.xz
License:	GPL,PD
# freetype2, xorg-x11 for "bdftopcf"
BuildRequires:	libHX-devel >= 3.4, freetype2, libcap-devel
BuildRequires:	xorg-x11, pkg-config, xz
BuildRoot:	%_tmppath/%name-%version-build
Recommends:	hxtools-data = %version
Prefix:		/opt/hxtools

%if "%{?vendor_uuid}" != ""
Provides:	%name(vendor:%vendor_uuid) = %version-%release
%endif

%description
A big tool collection.

%package data
Group:		System/Base
Summary:	Collection day-to-day tools (data)
Requires:	hxtools = %version
Obsoletes:	hxtools-noarch
BuildArch:	noarch

%description data
Architecture-indepent data for hxtools.

%prep
%setup -q

%build
%if "%{?use_fhs_paths}"
%cnofigure \
	--with-keymapdir=%_datadir/kbd/keymaps \
	--with-vgafontdir=%_datadir/kbd/consolefonts \
	--with-x11fontdir=%_datadir/fonts
%else
%configure \
	--prefix=%prefix \
	--exec-prefix=%prefix \
	--bindir=%prefix/bin \
	--libexecdir=%prefix/libexec \
	--with-keymapdir=%_datadir/kbd/keymaps \
	--with-vgafontdir=%_datadir/kbd/consolefonts \
	--with-x11fontdir=%_datadir/fonts
%endif
make %{?_smp_mflags};

%install
o="$PWD";
b="%buildroot";
rm -Rf "$b";
mkdir "$b";
%if "%{?use_fhs_paths}"
make install DESTDIR="$b";
%else
make install DESTDIR="$b" pkglibexecdir="%prefix/libexec";
%endif
install -dm0755 "$b/%_sysconfdir/openldap/schema" "$b/%_datadir/mc/syntax";
install -pm0644 cooledit/*.syntax "$b/%_datadir/mc/syntax/";
install -pm0644 data/rfc2307bis-utf8.schema "$b/%_sysconfdir/openldap/schema/";
cd "$b";
find opt/hxtools -type f -print0 | \
	xargs -0 grep -l ELF | perl -ne 'print"/$_"' >"$o/binary.lst";
find opt/hxtools -type f -print0 | \
	xargs -0 grep -L ELF | perl -ne 'print"/$_"' >"$o/data.lst";
chmod a+x "$b/%_sysconfdir"/hx*.bash;

%clean
rm -Rf "%buildroot";
echo -en "\e[1;32m""Now rebuild aaa_base if you changed DIR_COLORS!""\e[0m\n";

%files -f binary.lst
%defattr(-,root,root)
%dir %prefix
%dir %prefix/bin
%dir %prefix/libexec

%files data -f data.lst
%defattr(-,root,root)
%config %_sysconfdir/hx*
%dir %_sysconfdir/openldap
%dir %_sysconfdir/openldap/schema
%config %_sysconfdir/openldap/schema/*
%dir %prefix
%dir %prefix/bin
%dir %prefix/libexec
%_datadir/kbd
%_datadir/fonts/misc
%_datadir/mc
%doc %_mandir/*/*

%changelog
