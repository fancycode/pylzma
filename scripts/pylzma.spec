%define name pylzma
%define version 0.0.3
%define release 1
%define python python2.3

Summary: pylzma package
Name: %{name}
Version: %{version}
Release: %{release}
Source0: %{name}-%{version}.tar.gz
License: LGPL
Group: Development/Libraries
BuildRoot: %{_tmppath}/%{name}-buildroot
Prefix: %{_prefix}
Vendor: Joachim Bauch <mail@joachim-bauch.de>
Url: http://www.joachim-bauch.de
Requires: /usr/bin/python2.3

%description
Python bindings for the LZMA library by Igor Pavlov.

%prep
%setup

%build
env CFLAGS="$RPM_OPT_FLAGS" %{python} setup.py build

%install
%{python} setup.py install --root=$RPM_BUILD_ROOT --record=INSTALLED_FILES

%clean
rm -rf $RPM_BUILD_ROOT

%files -f INSTALLED_FILES
%defattr(-,root,root)
