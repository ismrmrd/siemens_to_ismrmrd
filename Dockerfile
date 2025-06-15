FROM ubuntu:latest as ismrmrd_base

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Chicago

RUN apt-get update && apt-get install -y git cmake g++ libhdf5-dev libxml2-dev libxslt1-dev libboost-dev libboost-program-options-dev libboost-system-dev libboost-filesystem-dev libboost-thread-dev libboost-timer-dev libboost-program-options-dev libpugixml-dev

RUN  mkdir -p /opt/code

RUN mkdir -p /opt/code/siemens_to_ismrmrd
COPY . /opt/code/siemens_to_ismrmrd/

# ISMRMRD library
RUN cd /opt/code && \
    git clone https://github.com/ismrmrd/ismrmrd.git && \
    cd ismrmrd && \
    git checkout $(cat /opt/code/siemens_to_ismrmrd/dependencies/ismrmrd | xargs) && \
    mkdir build && \
    cd build && \
    cmake ../ && \
    make -j $(nproc) && \
    make install

# siemens_to_ismrmrd converter
RUN cd /opt/code/siemens_to_ismrmrd && \
    mkdir build && \
    cd build && \
    cmake ../ && \
    make -j $(nproc) && \
    make install

# Create archive of ISMRMRD libraries (including symlinks) for second stage
RUN cd /usr/local/lib && tar -czvf libismrmrd.tar.gz libismrmrd*

# ----- Start another clean build without all of the build dependencies of siemens_to_ismrmrd -----
FROM ubuntu:latest

RUN apt-get update && apt-get install -y --no-install-recommends libxslt1.1 libhdf5-dev libpugixml-dev && apt-get clean && rm -rf /var/lib/apt/lists/*

# Copy siemens_to_ismrmrd from last stage and re-add necessary dependencies
COPY --from=ismrmrd_base /usr/local/bin/siemens_to_ismrmrd  /usr/local/bin/siemens_to_ismrmrd
COPY --from=ismrmrd_base /usr/local/lib/libismrmrd.tar.gz   /usr/local/lib/
RUN cd /usr/local/lib && tar -zxvf libismrmrd.tar.gz && rm libismrmrd.tar.gz && ldconfig
