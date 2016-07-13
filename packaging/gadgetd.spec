Name:           gadgetd
Version:        0.1.0
Release:        0
License:        Apache-2.0
Summary:        USB gadget daemon
Group:          Base/Device Management
Source0:        gadgetd-%{version}.tar.gz
Source1001:     gadgetd.manifest
BuildRequires:  cmake
BuildRequires:  pkg-config
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(libusbgx)

%description
Gadgetd is a tool for USB gadget management.

%package -n libffs-daemon
License:        Apache-2.0
Summary:        Library for ffs services

%description -n libffs-daemon
Library for all ffs services which should be activated on demand.

%package -n libffs-daemon-devel
License:        Apache-2.0
Summary:        Development package for ffs services

%description -n libffs-daemon-devel
Develiopment library for all ffs services which should be activated on demand.

%package -n libffs-daemon-examples
License:        Apache-2.0
Summary:        Sample applications using libffs-daemon
BuildRequires:  pkgconfig(libusb-1.0)
BuildRequires:  libaio-devel
BuildRequires:  libffs-daemon-devel

%description -n libffs-daemon-examples
Sample applications which demonstrates how to use libffs-daemon to write
ffs services

%prep
%setup -q
cp %{SOURCE1001} .

cmake . -DSUPPORT_FFS_LEGACY_API=1 -DBUILD_EXAMPLES=1

%build
make

%install


%make_install


%post -n libffs-daemon -p /sbin/ldconfig

%postun -n libffs-daemon -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%defattr(-,root,root)
%license LICENSE
/usr/local/bin/gadgetd
/etc/dbus-1/system.d/org.usb.gadgetd.conf

%files -n libffs-daemon
/usr/local/lib/libffs-daemon.so.0
/usr/local/lib/libffs-daemon.so.0.0.0

%files -n libffs-daemon-devel
/usr/local/include/gadgetd/ffs-daemon.h
/usr/local/lib/libffs-daemon.so

%files -n libffs-daemon-examples
/usr/local/bin/ffs-host-example
/usr/local/bin/ffs-service-example
/etc/gadgetd/functions.d/ffs.sample.example

