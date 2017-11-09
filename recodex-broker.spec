%define name recodex-broker
%define short_name broker
%define version 1.0.0
%define unmangled_version 1bd664ddc7ba30e7666ce0737f5a714ecbce9ae8
%define release 6

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
BuildRequires: systemd cmake zeromq-devel cppzmq-devel yaml-cpp-devel libcurl-devel
Requires(post): systemd
Requires(preun): systemd
Requires(postun): systemd

#Source0: %{name}-%{unmangled_version}.tar.gz
Source0: https://github.com/ReCodEx/%{short_name}/archive/%{unmangled_version}.tar.gz#/%{short_name}-%{unmangled_version}.tar.gz
Source1: https://github.com/gabime/%{spdlog_name}/archive/v%{spdlog_version}.tar.gz#/%{spdlog_name}-%{spdlog_version}.tar.gz

%description
Backend part of ReCodEx programmer testing solution.

%prep
%setup -n %{short_name}-%{unmangled_version}
# Unpack spdlog to the right location
%setup -n %{short_name}-%{unmangled_version} -T -D -a 1
rmdir vendor/spdlog
mv -f %{spdlog_name}-%{spdlog_version} vendor/spdlog

%build
%cmake -DDISABLE_TESTS=true .
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}
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
%config(noreplace) %attr(-,recodex,recodex) %{_sysconfdir}/recodex/broker/config.yml

#%{_unitdir}/recodex-broker.service
/lib/systemd/system/recodex-broker.service
/etc/munin/plugins/munin-broker

%changelog

