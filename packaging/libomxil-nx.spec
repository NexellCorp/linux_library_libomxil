Name:    libomxil-nx
Version: 0.0.1
Release: 0
License: LGPLv2+
Summary: Nexell OpenMax library
Group: Development/Libraries
Source:  %{name}-%{version}.tar.gz

BuildRequires:  pkgconfig(glib-2.0)

Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
Nexell OpenMax library

%package devel
Summary: Nexell OpenMax library
Group: Development/Libraries
License: LGPLv2+
Requires: %{name} = %{version}-%{release}

%description devel
Nexell OpenMax library (devel)

%prep
%setup -q

%build
make all NATIVE_BUILD=yes SONAME=libomxil-nx

%postun -p /sbin/ldconfig

%install
rm -rf %{buildroot}

mkdir -p %{buildroot}/usr/include
cp %{_builddir}/%{name}-%{version}/include/*.h  %{buildroot}/usr/include

mkdir -p %{buildroot}/usr/lib
cp %{_builddir}/%{name}-%{version}/lib/*  %{buildroot}/usr/lib

%files
%attr (0644, root, root) %{_libdir}/*
%license LICENSE.LGPLv2+

%files devel
%attr (0644, root, root) %{_includedir}/*
%license LICENSE.LGPLv2+
