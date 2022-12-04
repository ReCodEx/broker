%define name recodex-broker
%define short_name broker
%define version 1.4.0
%define unmangled_version f9710b51e97931696c92a2beb6e8f30a037b07b0
%define release 2

%define spdlog_name spdlog
%define spdlog_version 0.13.0

Summary: ReCodEx broker component
Name: %{name}
Version: %{version}
Release: %{release}
License: MIT
Group: Development/Libraries
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Prefix: %{_prefix}
Vendor: Petr Stefan <UNKNOWN>
Url: https://github.com/ReCodEx/broker
BuildRequires: systemd gcc-c++ cmake zeromq-devel cppzmq-devel yaml-cpp-devel libcurl-devel boost-devel
Requires(post): systemd
Requires(preun): systemd
Requires(postun): systemd

#Source0: %{name}-%{unmangled_version}.tar.gz
Source0: https://github.com/ReCodEx/%{short_name}/archive/%{unmangled_version}.tar.gz#/%{short_name}-%{unmangled_version}.tar.gz
Source1: https://github.com/gabime/%{spdlog_name}/archive/v%{spdlog_version}.tar.gz#/%{spdlog_name}-%{spdlog_version}.tar.gz

%description
Middleware for ReCodEx code examiner, an educational application for evaluating programming assignments. The broker manages communication between frontend (core-api) and backend workers.

%prep
%setup -n %{short_name}-%{unmangled_version}
# Unpack spdlog to the right location
%setup -n %{short_name}-%{unmangled_version} -T -D -a 1
rmdir vendor/spdlog
mv -f %{spdlog_name}-%{spdlog_version} vendor/spdlog

%build
%cmake -DDISABLE_TESTS=true .
%cmake_build

%install
%cmake_install --prefix %{buildroot}
mkdir -p %{buildroot}/var/log/recodex

%clean


%post
%systemd_post 'recodex-broker.service'

%postun
%systemd_postun_with_restart 'recodex-broker.service'

%pre
getent group recodex >/dev/null || groupadd -r recodex
getent passwd recodex >/dev/null || useradd -r -g recodex -d %{_sysconfdir}/recodex -s /sbin/nologin -c "ReCodEx Code Examiner" recodex
exit 0

%preun
%systemd_preun 'recodex-broker.service'

%files
%defattr(-,root,root)
%dir %attr(-,recodex,recodex) %{_sysconfdir}/recodex/broker
%dir %attr(-,recodex,recodex) /var/log/recodex

%{_bindir}/recodex-broker
%config(noreplace) %attr(0600,recodex,recodex) %{_sysconfdir}/recodex/broker/config.yml

#%{_unitdir}/recodex-broker.service
/lib/systemd/system/recodex-broker.service

%changelog


