
Name:		hxtools
Version:	20101005
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
install -dm0755 "$b/%_sysconfdir/openldap/schema" "$b/%_datadir/mc/syntax";
install -pm0644 cooledit/*.syntax "$b/%_datadir/mc/syntax/";
install -pm0644 data/rfc2307bis-utf8.schema "$b/%_sysconfdir/openldap/schema/";
cd "$b";
find * -type f ! -wholename "usr/share/man*" -print0 | \
	xargs -0 grep -l ELF | perl -ne 'print"/$_"' >"$o/binary.lst";
find * -type f ! -wholename "usr/share/man*" -print0 | \
	xargs -0 grep -L ELF | perl -ne 'print"/$_"' >"$o/data.lst";
chmod a+x "$b/%_sysconfdir"/hx*.bash;
ln "$b/%_sysconfdir/hxtools_dircolors" "$b/%_sysconfdir/DIR_COLORS";
mkdir -p "$b/%_sysconfdir/profile.d";
ln -s "../hxtools_profile.bash" "$b/%_sysconfdir/profile.d/z_hxtools_profile.sh";

%clean
rm -Rf "%buildroot";

%files -f binary.lst
%defattr(-,root,root)

%files data -f data.lst
%defattr(-,root,root)
%config %_sysconfdir/hx*
%config %_sysconfdir/DIR_COLORS
%config %_sysconfdir/profile.d/*
%dir %_sysconfdir/openldap
%dir %_sysconfdir/openldap/schema
%config %_sysconfdir/openldap/schema/*
%_datadir/%name
%_datadir/kbd
%_datadir/fonts/misc
%_datadir/mc
%doc %_mandir/*/*

%changelog
