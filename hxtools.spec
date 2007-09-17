
Name:		hxtools
Version:	20070917
Release:	0
Group:		System/Base
URL:		http://jengelh.hopto.org/
Summary:	Collection of day-to-day tools

Source:		%name-%version.tar.bz2
License:	GPL,PD
# freetype2, xorg-x11 for "bdftopcf"
BuildRequires:	libHX-devel >= 1.10, freetype2, xorg-x11
BuildRoot:	%_tmppath/%name-%version-build
Prefix:		/opt/hxtools

%description
A big tool collection.

%debug_package
%prep
%setup

%build
%configure \
	--prefix=/opt/hxtools \
	--exec-prefix=/opt/hxtools \
	--bindir=/opt/hxtools/bin \
	--with-suppdir=/opt/hxtools/supp \
	--with-keymapdir=%_datadir/kbd/keymaps \
	--with-vgafontdir=%_datadir/kbd/consolefonts \
	--with-x11fontdir=%_datadir/fonts
make %{?jobs:-j%jobs};

%install
b="%buildroot";
rm -Rf "$b";
mkdir "$b";
make install DESTDIR="$b";

%clean
rm -Rf "%buildroot";

%files
%defattr(-,root,root)
%_datadir/kbd/consolefonts/*
%_datadir/kbd/keymaps/i386/*/*
/opt/hxtools
%_datadir/fonts/misc/*
%doc doc/*

%changelog -n hxtools
