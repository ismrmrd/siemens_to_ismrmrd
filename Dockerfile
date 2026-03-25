FROM mcr.microsoft.com/devcontainers/base:jammy AS ismrmrd_dev

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
        git cmake g++ pkg-config \
        libboost-all-dev libpugixml-dev \
        python3-venv \
    && apt-get clean

RUN curl -fsSL https://just.systems/install.sh | bash -s -- --to /usr/local/bin

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

FROM ismrmrd_dev AS siemens_to_ismrmrd_dev

# libxml2
RUN cd /opt/code \
    && wget https://download.gnome.org/sources/libxml2/2.14/libxml2-2.14.5.tar.xz \
    && tar xJf libxml2-2.14.5.tar.xz \
    && cd libxml2-2.14.5/ \
    && ./configure --enable-static=yes --without-python \
    && make install

# libxslt
RUN cd /opt/code \
    && wget https://download.gnome.org/sources/libxslt/1.1/libxslt-1.1.43.tar.xz \
    && tar xJf libxslt-1.1.43.tar.xz \
    && cd libxslt-1.1.43 \
    && ./configure --enable-static=yes --without-python \
    && make install

FROM siemens_to_ismrmrd_dev AS siemens_to_ismrmrd_build

# siemens_to_ismrmrd converter
RUN cd /opt/code/siemens_to_ismrmrd && \
    mkdir build && \
    cd build && \
    cmake ../ && \
    make -j $(nproc) && \
    make install

RUN cd /opt/code/siemens_to_ismrmrd/test && \
    python3 -m venv .venv && \
    . .venv/bin/activate && \
    pip install -r requirements.txt && \
    python -m pytest --download-all --echo-log-on-failure

FROM mcr.microsoft.com/devcontainers/base:jammy AS siemens_to_ismrmrd
RUN apt-get update && apt-get clean && rm -rf /var/lib/apt/lists/*
COPY --from=siemens_to_ismrmrd_build /usr/local/bin/siemens_to_ismrmrd /usr/local/bin/siemens_to_ismrmrd
COPY --from=siemens_to_ismrmrd_build /usr/local/bin/ismrmrd_to_siemens /usr/local/bin/ismrmrd_to_siemens