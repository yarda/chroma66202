%global git_commit 3b53d6e7eabec00873942cc83db05cbe13aaf8d1
%global git_date 20181121

%global git_short_commit %(echo %{git_commit} | cut -c -8)
%global git_suffix %{git_date}git%{git_short_commit}

# git clone git://github.com/yarda/chroma66202.git
# cd %%{name}
# git archive --format=tar --prefix=%%{name}-%%{version}/ %%{git_commit} | \
# bzip2 > ../%%{name}-%%{version}-%%{git_suffix}.tar.bz2

Name:             chroma66202
URL:              https://github.com/yarda/chroma66202
Version:          0
Release:          0.2.%{git_suffix}%{?dist}
License:          GPLv3+
Summary:          Utilities for Chroma 66202 digital power meter
BuildRequires:    gcc, make, coreutils, sed
Source0:          %{url}/archive/%{git_commit}/%{name}-%{version}-%{git_suffix}.tar.gz

%description
Utilities for Chroma 66202 digital power meter.

%package wx
Summary:          GUI tools for Chroma 66202 and Watchport
Requires:         %{name} = %{version}-%{release}
Requires:         python-matplotlib-wx
BuildRequires:    python2-devel
BuildArch:        noarch

%description wx
GUI tools for Chroma 66202 digital power meter and Watchport
temperature / humidity sensor.

%prep
%setup -q -n %{name}-%{git_commit}

%build
%make_build CFLAGS="%{optflags}" LDFLAGS="%{?__global_ldflags}"

%install
%make_install BINDIR="%{_bindir}"
install -p -m 0755 pwr_graph.py %{buildroot}%{_bindir}/pwr_graph
install -p -m 0755 temp_graph.py %{buildroot}%{_bindir}/temp_graph
for f in pwr_graph temp_graph
do
  sed -i  '1 s|#!/usr/bin/python|#!%{__python2}|' "%{buildroot}%{_bindir}/$f"
done

%files
%doc README
%{_bindir}/chroma66202
%{_bindir}/pwrtest


%files wx
%{_bindir}/pwr_graph
%{_bindir}/temp_graph


%changelog
* Wed Nov 21 2018 Jaroslav Škarvada <jskarvad@redhat.com> - 0-0.2.20181121git3b53d6e7
- New version
- Improved SPEC

* Tue Apr 16 2013 Jaroslav Škarvada <jskarvad@redhat.com> - 0-0.1.20130416git2192b1db
- Initial version
