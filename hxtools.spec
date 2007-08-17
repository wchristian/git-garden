
Name:           hxtools
Version:        20070817
Release:        ccj0
Group:          System/Base
URL:            http://jengelh.hopto.org/
Summary:        Collection of day-to-day tools

Source:         %name-%version.tar.bz2
License:        GPL,PD
BuildRequires:	libHX >= 1.10
BuildRoot:      %_tmppath/%name-%version-build
Prefix:         /opt/hxtools

%description
A big tool collection.

%debug_package
%prep
%setup

%build
./autogen.sh;
%configure --prefix=%_prefix \
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
%_libdir/%name
%_datadir/fonts/misc/*
%doc doc/*

%changelog -n hxtools
