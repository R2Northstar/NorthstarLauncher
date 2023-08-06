FROM registry.fedoraproject.org/fedora-toolbox:38

RUN dnf update -y && \
    dnf install -y \
    	git \
    	wine \
	wine-mono \
	python3 \
	msitools \
	python3-simplejson \
	python3-six \
	cmake \
	ninja-build \
	make \
	samba \
	libunwind



RUN mkdir /opt/msvc/ && \
    mkdir /build/
RUN git clone https://github.com/mstorsjo/msvc-wine
RUN ./msvc-wine/vsdownload.py --accept-license --dest /opt/msvc/
RUN ./msvc-wine/install.sh /opt/msvc/
RUN rm -rf ~/.wine
RUN git config --global --add safe.directory '/build'
ENV PATH="/opt/msvc/bin/x64:${PATH}"
WORKDIR /build/
