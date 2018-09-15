# Copyright 2018 The SAF Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Dokerfile for building SAF with OpenVINO (CVSDK)
#
#   Usage: sudo docker build -f Dockerfile . -t saf \
#                            --build-arg USE_VDMS=ON \
#                            --build-arg GITHUB_TOKEN=<OAuth-string> \
#                            --build-arg GITHUB_URL=<GitHub-URL>
#                            --build-arg SAF_HASH=<commit-id>
#
#   When building under within VPN, add the following to your build command:
#     --build-arg http_proxy=<protocol>://<hostname>:<port>
#     --build-arg https_proxy=<protocol>://<hostname>:<port>
#
#   To run, use `docker run -it saf`
#            or `sudo docker run -it --device /dev/dri:/dev/dri saf`
#
#   Prebuilt Docker images are available upon requests.

FROM ubuntu:16.04
MAINTAINER Shao-Wen Yang <shao-wen.yang@intel.com>

ENV DEBIAN_FRONTEND noninteractive

ARG DEFAULT_WORKDIR=/vcs
ARG USE_VDMS=OFF
ARG GITHUB_URL=github.com/viscloud/saf.git
ARG GITHUB_TOKEN=
ARG SAF_HASH=

# Prepare toolchain

RUN apt-get update && apt-get install --yes --no-install-recommends dialog \
    apt-utils
RUN apt-get install --yes --no-install-recommends build-essential
RUN apt-get install --yes --no-install-recommends pkg-config
RUN apt-get install --yes --no-install-recommends g++ wget cmake git vim
RUN apt-get install --yes --no-install-recommends ca-certificates
RUN apt-get install --yes --no-install-recommends autoconf automake libtool
RUN apt-get install --yes --no-install-recommends curl unzip
RUN apt-get install --yes --no-install-recommends \
    libavcodec-dev:amd64 \
    libavformat-dev:amd64 \
    libswscale-dev:amd64 \
    libtbb2:amd64 \
    libtbb-dev:amd64 \
    libjpeg-dev:amd64 \
    libpng12-dev:amd64 \
    libjasper-dev \
    libdc1394-22-dev:amd64
RUN apt-get install --yes --no-install-recommends libeigen3-dev
RUN apt-get install --yes --no-install-recommends libboost-all-dev
RUN apt-get install --yes --no-install-recommends \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-good1.0-dev \
    libgstreamer-plugins-bad1.0-dev \
    gstreamer1.0:amd64
RUN apt-get install --yes --no-install-recommends \
    python-scipy \
    python-numpy \
    python-yaml
RUN apt-get install --yes --no-install-recommends \
    libleveldb-dev:amd64 \
    libsnappy-dev:amd64 \
    libhdf5-serial-dev \
    libgflags-dev \
    libgoogle-glog-dev \
    liblmdb-dev:amd64 \
    libjemalloc-dev \
    libzmq3-dev:amd64
RUN apt-get install --yes --no-install-recommends libblas-dev
RUN apt-get install --yes --no-install-recommends libgtk2.0-dev
RUN apt-get install --yes --no-install-recommends libcpprest-dev
RUN apt-get install --yes --no-install-recommends libopenblas-dev
RUN apt-get install --yes --no-install-recommends \
    alien \
    clinfo \
    opencl-headers
RUN apt-get install --yes --no-install-recommends libjsoncpp-dev

# Prepare the environment

RUN mkdir -p $DEFAULT_WORKDIR
ENV PATH ${PATH}:/usr/local/bin
ENV LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:/usr/local/lib

# OpenVINO (CVSDK)

WORKDIR $DEFAULT_WORKDIR
RUN wget http://registrationcenter-download.intel.com/akdlm/irc_nas/13131/l_openvino_toolkit_p_2018.1.265.tgz
RUN tar zxvf l_openvino_toolkit_p_2018.1.265.tgz && \
    cd l_openvino_toolkit_p_2018.1.265 && \
    sed -i 's/ACCEPT_EULA=decline/ACCEPT_EULA=accept/g' silent.cfg && \
    ./install.sh -s silent.cfg
ENV LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:/opt/intel/opencl
RUN apt-get install --yes --no-install-recommends lsb-release
RUN /bin/bash -c "source /opt/intel/computer_vision_sdk/bin/setupvars.sh"
ENV PATH /opt/intel/computer_vision_sdk/deployment_tools/model_optimizer:${PATH}
ENV LD_LIBRARY_PATH /usr/local/lib:/opt/intel/computer_vision_sdk/opencv/share/OpenCV/3rdparty/lib:/opt/intel/computer_vision_sdk/opencv/lib:/opt/intel/opencl:/opt/intel/computer_vision_sdk/deployment_tools/inference_engine/external/cldnn/lib:/opt/intel/computer_vision_sdk/deployment_tools/inference_engine/external/mkltiny_lnx/lib:/opt/intel/computer_vision_sdk/deployment_tools/inference_engine/lib/ubuntu_16.04/intel64:/opt/intel/computer_vision_sdk/deployment_tools/model_optimizer/model_optimizer_caffe/bin:/opt/intel/computer_vision_sdk/openvx/lib::/opt/intel/opencl
ENV PYTHONPATH /opt/intel/computer_vision_sdk/deployment_tools/model_optimizer:${PYTHONPATH}
ENV OpenCV_DIR /opt/intel/computer_vision_sdk/opencv/share/OpenCV
RUN cd /opt/intel/computer_vision_sdk/install_dependencies && \
    ./install_NEO_OCL_driver.sh
RUN cd /opt/intel/computer_vision_sdk/deployment_tools/inference_engine/samples && \
    mkdir build && cd build && cmake .. && make cpu_extension -j4
WORKDIR $DEFAULT_WORKDIR
RUN rm -rf l_openvino_toolkit_p_2018.1.265 l_openvino_toolkit_p_2018.1.265.tgz

# Protobuf

WORKDIR $DEFAULT_WORKDIR
RUN git clone https://github.com/google/protobuf.git
RUN cd protobuf && \
    git reset --hard 072431452a365450c607e9503f51786be44ecf7f && \
    ./autogen.sh && \
    ./configure --disable-shared --with-pic && \
    make -j4 && make install && cd .. && rm -rf protobuf

# VDMS

WORKDIR $DEFAULT_WORKDIR
RUN if [ ON = "$USE_VDMS" ]; then \
        apt-get install --yes --no-install-recommends scons flex && \
        apt-get install --yes --no-install-recommends javacc openjdk-8-jdk && \
        apt-get install --yes --no-install-recommends bison libbison-dev && \
        apt-get install --yes --no-install-recommends zlib1g-dev && \
        apt-get install --yes --no-install-recommends libbz2-dev && \
        apt-get install --yes --no-install-recommends libssl-dev && \
        apt-get install --yes --no-install-recommends liblz4-dev && \
        apt-get install --yes --no-install-recommends mpich && \
        apt-get install --yes --no-install-recommends libopenmpi-dev && \
        apt-get install --yes --no-install-recommends libgtest-dev ed && \
        apt-get install --yes --no-install-recommends libtbb2 libtbb-dev && \
        apt-get install --yes --no-install-recommends libdc1394-22-dev && \
        git clone https://github.com/Blosc/c-blosc.git && \
        cd $DEFAULT_WORKDIR/c-blosc && mkdir build && cd build && cmake .. && \
        cmake --build . && ctest && cmake --build . --target install && \
        cd ../.. && rm -rf c-blosc && \
        cd $DEFAULT_WORKDIR && \
        wget https://github.com/facebook/zstd/archive/v1.1.0.tar.gz && \
        tar xf v1.1.0.tar.gz && cd zstd-1.1.0 && \
        make -j4 && make install && \
        cd .. && rm -f v1.1.0.tar.gz && rm -rf zstd-1.1.0 && \
        cd /usr/src/gtest && cmake . && make -j4 && \
        mv libgtest* /usr/local/lib/ && \
        cd $DEFAULT_WORKDIR && \
        wget https://github.com/TileDB-Inc/TileDB/archive/0.6.1.tar.gz && \
        tar xf 0.6.1.tar.gz && cd TileDB-0.6.1 && \
        mkdir build && cd build && cmake .. && make -j4 && make install && \
        cd ../.. && rm -f 0.6.1.tar.gz && rm -rf TileDB-0.6.1 && \
        git clone https://github.com/tristanpenman/valijson.git && \
        cd $DEFAULT_WORKDIR/valijson && cp -r include/* /usr/local/include && \
        cd .. && rm -rf valijson && \
        cd $DEFAULT_WORKDIR && \
        wget https://github.com/intellabs/vcl/archive/v0.1.0.tar.gz && \
        tar xf v0.1.0.tar.gz && mv vcl-0.1.0 vcl && \
        cd vcl && sed -i "s/\(CPPPATH\s*=\s*\[.*\)\(\]\)/\1,\'\/opt\/intel\/computer_vision_sdk\/opencv\/include\'\2/g" SConstruct && sed -i "s/\(LIBPATH\s*=\s*\[.*\)\(\]\)/\1,\'\/opt\/intel\/computer_vision_sdk\/opencv\/lib\'\2/g" SConstruct&& scons -j4 && \
        cd .. && rm -f v0.1.0.tar.gz && \
        cd $DEFAULT_WORKDIR && \
        wget https://github.com/intellabs/pmgd/archive/v1.0.0.tar.gz && \
        tar xf v1.0.0.tar.gz && mv pmgd-1.0.0 pmgd && cd pmgd && make -j4 && \
        cd .. && rm -f v1.0.0.tar.gz && \
        cd $DEFAULT_WORKDIR && \
        wget https://github.com/intellabs/vdms/archive/v1.0.0.tar.gz && \
        tar xf v1.0.0.tar.gz && mv vdms-1.0.0 vdms && cd vdms && \
        sed -i "s/CPPPATH\s*.*\[/&\'\/opt\/intel\/computer_vision_sdk\/opencv\/include\',/g" SConstruct && \
        sed -i "s/LIBPATH\s*.*\[/&\'\/opt\/intel\/computer_vision_sdk\/opencv\/lib\',/g" SConstruct && \
        sed -i "s/LIBS\s*.*\[/&\'opencv_core\',\'opencv_imgproc\',\'opencv_imgcodecs\',/g" SConstruct && \
        mkdir db && scons -j4 INTEL_PATH=$DEFAULT_WORKDIR && \
        cd .. && rm -f v1.0.0.tar.gz; \
    fi

# SAF

WORKDIR $DEFAULT_WORKDIR
ENV no_proxy "github.intel.com localhost"
RUN if [ ! -z "$GITHUB_TOKEN" ]; then \
        git clone https://$GITHUB_TOKEN@$GITHUB_URL saf; \
    else \
        git clone https://$GITHUB_URL saf; \
    fi
RUN cd saf && \
    if [ ! -z "$SAF_HASH" ]; then git reset --hard $SAF_HASH; fi && \
    if [ ON = "$USE_VDMS" ]; then \
        mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DUSE_CVSDK=ON -DBACKEND=cpu -DBUILD_TESTS=OFF -DUSE_SSD=ON -DUSE_WEBSOCKET=ON -DUSE_KAFKA=ON -DUSE_MQTT=ON -DUSE_PYTHON=ON -DUSE_VDMS=ON -DVDMS_HOME=$DEFAULT_WORKDIR/vdms ..; \
    else \
        mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DUSE_CVSDK=ON -DBACKEND=cpu -DBUILD_TESTS=OFF -DUSE_SSD=ON -DUSE_WEBSOCKET=ON -DUSE_KAFKA=ON -DUSE_MQTT=ON -DUSE_PYTHON=ON ..; \
    fi && \
    make -j4 && make apps -j4 && make install_python

# Set up SAF 

WORKDIR $DEFAULT_WORKDIR/saf/config
RUN cp config.toml.example config.toml && \
    cp cameras.toml.example cameras.toml && \
    cp models.toml.example models.toml && \
    echo "\n[[model]]\n\
name = \"person-detection-retail-0012\"\n\
type = \"cvsdk\"\n\
desc_path = \"/opt/intel/computer_vision_sdk/deployment_tools/intel_models/person-detection-retail-0012/FP32/person-detection-retail-0012.xml\"\n\
params_path = \"/opt/intel/computer_vision_sdk/deployment_tools/intel_models/person-detection-retail-0012/FP32/person-detection-retail-0012.bin\"\n\
input_width = 96\n\
input_height = 112\n\
label_file = \"/opt/intel/computer_vision_sdk/deployment_tools/intel_models/person-detection-retail-0012/label.names\"" >> models.toml && \
    echo "person" >> /opt/intel/computer_vision_sdk/deployment_tools/intel_models/person-detection-retail-0012/label.names

# Prepare sample scripts

WORKDIR $DEFAULT_WORKDIR/saf
RUN echo "#!/bin/sh\n\
build/apps/simple --camera GST_TEST" > run_simple.sh && chmod +x run_simple.sh
RUN echo "#!/bin/sh\n\
CAMERA=GST_TEST\n\
HOST=localhost\n\
if [ -n \"\$1\" ]; then CAMERA=\$1; fi\n\
if [ -n \"\$2\" ]; then HOST=\$2; fi\n\
build/apps/detector -c \$CAMERA --detector_type cvsdk-ssd -m person-detection-retail-0012 --detector_targets person --detector_confidence_threshold 0.3 --sender_package_type frame --sender_endpoint \"kafka://\$HOST:9092\"" > run_detector.sh && chmod +x run_detector.sh
RUN echo "#!/bin/sh\n\
CAMERA=GST_TEST\n\
HOST=localhost\n\
if [ -n \"\$1\" ]; then CAMERA=\$1; fi\n\
if [ -n \"\$2\" ]; then HOST=\$2; fi\n\
build/apps/tracker -c \$CAMERA --detector_type cvsdk-ssd -m person-detection-retail-0012 --detector_targets person --detector_confidence_threshold 0.3 --detector_idle_duration 0.1 --tracker_type kf --sender_package_type frame --sender_endpoint \"kafka://\$HOST:9092\"" > run_tracker.sh && chmod +x run_tracker.sh
RUN echo "#!/bin/sh\n\
CAMERA=GST_TEST\n\
HOST=localhost\n\
if [ -n \"\$1\" ]; then CAMERA=\$1; fi\n\
if [ -n \"\$2\" ]; then HOST=\$2; fi\n\
build/apps/visualizer --sender_endpoint \"kafka://\$HOST:9092\" -c \$CAMERA" > run_visualizer.sh && chmod +x run_visualizer.sh
RUN echo "#!/bin/sh\n\
CAMERA=GST_TEST\n\
HOST=localhost\n\
if [ -n \"\$1\" ]; then CAMERA=\$1; fi\n\
if [ -n \"\$2\" ]; then HOST=\$2; fi\n\
build/apps/writer --sender_endpoint \"kafka://\$HOST:9092\" --write_target file --write_uri sample.csv -c \$CAMERA" > run_writer.sh && chmod +x run_writer.sh

# Install OpenCV-Python for better Python support

RUN apt-get install --yes --no-install-recommends python-pip && \
    pip install opencv-python

# Resolve ProtoBuf version conflict
# In this script, ProtoBuf3 is linked against statically.
# The following removes ProtoBuf3 shared libraries.
# Do NOT EVER do the following on your native setup.

RUN rm -f /usr/lib/x86_64-linux-gnu/libprotobuf*

# Clean up

RUN apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

WORKDIR $DEFAULT_WORKDIR/saf
