FROM mcr.microsoft.com/devcontainers/base:jammy AS ismrmrd_dev

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
        git cmake g++ pkg-config \
        libboost-dev libboost-program-options-dev libboost-system-dev \
        libboost-filesystem-dev libboost-thread-dev libboost-timer-dev \
        libboost-program-options-dev libboost-test-dev libpugixml-dev \
    && apt-get clean

RUN mkdir -p /opt/code/siemens_to_ismrmrd
COPY . /opt/code/siemens_to_ismrmrd/

# ISMRMRD library
RUN cd /opt/code && \
    git clone https://github.com/ismrmrd/ismrmrd.git && \
    cd ismrmrd && \
    git checkout $(cat /opt/code/siemens_to_ismrmrd/dependencies/ismrmrd | xargs) && \
    mkdir build && \
    cd build && \
    cmake -D CMAKE_BUILD_TYPE=Release -D USE_HDF5_DATASET_SUPPORT=Off -D BUILD_STATIC=On ../ && \
    make -j $(nproc) && \
    make install

# libxml2
RUN cd /opt/code && \
    wget ftp://xmlsoft.org/libxslt//libxml2-2.9.12.tar.gz && \
    tar xzf libxml2-2.9.12.tar.gz && \
    cd libxml2-2.9.12/ && \
    ./configure && \
    make install

# libxslt
RUN cd /opt/code && \
    wget ftp://xmlsoft.org/libxslt//libxslt-1.1.34.tar.gz && \
    tar xzf libxslt-1.1.34.tar.gz && \
    cd libxslt-1.1.34 && \
    ./configure && \
    make install

FROM ismrmrd_dev AS siemens_to_ismrmrd_dev

# siemens_to_ismrmrd converter
RUN cd /opt/code/siemens_to_ismrmrd && \
    mkdir build && \
    cd build && \
    cmake ../ && \
    make -j $(nproc) && \
    make install

FROM mcr.microsoft.com/devcontainers/base:jammy AS siemens_to_ismrmrd
RUN apt-get update && apt-get clean && rm -rf /var/lib/apt/lists/*
COPY --from=siemens_to_ismrmrd_dev /usr/local/bin/siemens_to_ismrmrd /usr/local/bin/siemens_to_ismrmrd