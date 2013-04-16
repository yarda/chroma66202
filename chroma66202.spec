%global git_commit 2192b1db3020de053d63acf2514b81cdff6caf53
%global git_date 20130416

%global git_short_commit %(echo %{git_commit} | cut -c -8)
%global git_suffix %{git_date}git%{git_short_commit}

# git clone git://github.com/yarda/chroma66202.git
# cd %%{name}
# git archive --format=tar --prefix=%%{name}-%%{version}/ %%{git_commit} | \
# bzip2 > ../%%{name}-%%{version}-%%{git_suffix}.tar.bz2

Name:             chroma66202
URL:              http://github.com/yarda/chroma66202.git
Version:          0
Release:          0.1.%{git_suffix}%{?dist}
License:          GPLv3+
Group:            Applications/Engineering
Summary:          Utilities for Chroma 66202 digital power meter
Source0:          %{name}-%{version}-%{git_suffix}.tar.bz2

%description
Utilities for Chroma 66202 digital power meter.

%package wx
Summary:          GUI tools for Chroma 66202 and Watchport
Group:            Applications/Communications
Requires:         %{name} = %{version}-%{release}
Requires:         python-matplotlib-wx
BuildArch:        noarch

%description wx
GUI tools for Chroma 66202 digital power meter and Watchport
temperature / humidity sensor.

%prep
%setup -q

%build
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot} BINDIR=%{_bindir}
install -p -m 0755 pwr_graph.py %{buildroot}%{_bindir}/pwr_graph
install -p -m 0755 temp_graph.py %{buildroot}%{_bindir}/temp_graph

%files
%doc README
%{_bindir}/chroma66202
%{_bindir}/pwrtest


%files wx
%{_bindir}/pwr_graph
%{_bindir}/temp_graph


%changelog
* Tue Apr 16 2013 Jaroslav Škarvada <jskarvad@redhat.com> - 0-0.1.20130416git2192b1db
- Initial version
