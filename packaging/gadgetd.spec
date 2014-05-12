Name:           gadgetd
Version:        0.1.0
Release:        0
License:        Apache License, Version 2.0
Summary:        USB gadget daemon
Group:          Base/Device Management
Source0:        gadgetd-%{version}.tar.gz
Source1001:     gadgetd.manifest
BuildRequires:  cmake
BuildRequires:  pkg-config
BuildRequires:  libusbg-devel
BuildRequires:  pkgconfig(glib-2.0)
Requires:       libusbg-0.1.0


%description
Gadgetd is a tool for USB gadget management.

%prep
%setup -q
cp %{SOURCE1001} .

cmake .

%build
make

%install


%make_install


%post

%postun

%files
%manifest %{name}.manifest
%defattr(-,root,root)
%license LICENSE
/usr/local/bin/gadgetd
