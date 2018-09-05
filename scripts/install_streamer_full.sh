#!/bin/bash

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

# Install SAF in one shot

if [ $# -ne 1 ]; then
  echo "Usage: install_saf_full.sh <path-to-workdir>"
  echo "Example: ./install_saf_full.sh ~/workdir"
  exit 2
fi

if [ ! -d "${1}" ]; then
  echo "Usage: install_saf_full.sh <path-to-workdir>"
  echo "Example: ./install_saf_full.sh ~/workdir"
  exit 2
fi

read -s -p "Enter Password for sudo: " sudoPW

WORKDIR=${1}

echo $sudoPW | sudo -S apt-get update
echo $sudoPW | sudo -S apt-get install -y \
	realpath \
	wget=1.17.1-1ubuntu1.1 \
	cmake=3.5.1-1ubuntu3 \
	git=1:2.7.4-0ubuntu1.2 \
	vim=2:7.4.1689-3ubuntu1.2 \
	autoconf=2.69-9 \
	automake=1:1.15-4ubuntu1 \
	libtool=2.4.6-0.1 \
	curl=7.47.0-1ubuntu2.2 \
	g++=4:5.3.1-1ubuntu1 \
	unzip=6.0-20ubuntu1 \
	build-essential=12.1ubuntu2 \
	ocl-icd-opencl-dev:amd64=2.2.8-1 \
	libavcodec-dev:amd64=7:2.8.11-0ubuntu0.16.04.1 \
	libavformat-dev:amd64=7:2.8.11-0ubuntu0.16.04.1 \
	libswscale-dev:amd64=7:2.8.11-0ubuntu0.16.04.1 \
	python-dev=2.7.11-1 \
	python-numpy=1:1.11.0-1ubuntu1 \
	libtbb2:amd64=4.4~20151115-0ubuntu3 \
	libtbb-dev:amd64=4.4~20151115-0ubuntu3 \
	libjpeg-dev:amd64=8c-2ubuntu8 \
	libpng12-dev:amd64=1.2.54-1ubuntu1 \
	libtiff-dev \
	libjasper-dev=1.900.1-debian1-2.4ubuntu1.1 \
	libdc1394-22-dev:amd64=2.2.4-1 \
	libopenexr-dev=2.2.0-10ubuntu2 \
	libeigen3-dev=3.3~beta1-2 \
	libgstreamer1.0-dev=1.8.3-1~ubuntu0.1 \
	libgstreamer-plugins-base1.0-dev=1.8.3-1ubuntu0.2 \
	libgstreamer-plugins-good1.0-dev=1.8.3-1ubuntu0.4 \
	libgstreamer-plugins-bad1.0-dev=1.8.3-1ubuntu0.2 \
	python3-dev=3.5.1-3 \
	libleveldb-dev:amd64=1.18-5 \
	libsnappy-dev:amd64=1.1.3-2 \
	libhdf5-serial-dev=1.8.16+docs-4ubuntu1 \
	libboost-all-dev=1.58.0.1ubuntu1 \
	libgflags-dev=2.1.2-3 \
	libgoogle-glog-dev=0.3.4-0.1 \
	liblmdb-dev:amd64=0.9.17-3 \
	python-scipy=0.17.0-1 \
	gstreamer1.0:amd64 \
	libjemalloc-dev=3.6.0-9ubuntu1 \
	libzmq3-dev:amd64=4.1.4-7 \
	libeigen3-dev=3.3~beta1-2 \
	libblas-dev=3.6.0-2ubuntu2 \
	libgtk2.0-dev=2.24.30-1ubuntu1.16.04.2 \
	python-pip=8.1.1-2ubuntu0.4 \
	libcpprest-dev=2.8.0-2;

pip install pyyaml

# Build & Install Protobuf v3
cd $WORKDIR
git clone https://github.com/google/protobuf.git; \
	cd protobuf; \
	git reset --hard 072431452a365450c607e9503f51786be44ecf7f; \
	./autogen.sh; ./configure; \
	make -j4; echo $sudoPW | sudo -S make install; cd ..; rm -rf protobuf

# Build OpenCV v3
cd $WORKDIR
git clone https://github.com/opencv/opencv.git; \
	cd opencv; \
	git reset --hard d34eec3ab3940c085b12d0420d9bbe76ea25e620; \
	mkdir build; cd build; \
	cmake -DCMAKE_BUILD_TYPE=RELEASE -DBUILD_NEW_PYTHON_SUPPORT=ON \
	-DWITH_OPENGL=ON -DWITH_OPENCL=ON -DWITH_EIGEN=ON \
	-DBUILD_PROTOBUF=OFF -DBUILD_opencv_dnn=OFF ..; \
	make;echo $sudoPW | sudo -S make install; cd ../../; rm -rf opencv

export PATH=${PATH}:/usr/local/bin
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib

# Build Intel-Caffe
cd $WORKDIR
git clone https://github.com/intel/caffe.git intel-caffe && \
cd intel-caffe && \
git reset --hard 5b1312465c75a0e2dd94e49cc99b7ef5a76594dc && \
sed -i 's/LIBRARIES += glog gflags protobuf m hdf5_hl hdf5/LIBRARIES += glog gflags protobuf m hdf5_serial_hl hdf5_serial/g' Makefile && \
cp Makefile.config.example Makefile.config && \
sed -i 's/# OPENCV_VERSION := 3/OPENCV_VERSION := 3/g' Makefile.config && \
sed -i 's/INCLUDE_DIRS := $(PYTHON_INCLUDE) \/usr\/local\/include/INCLUDE_DIRS := $(PYTHON_INCLUDE) \/usr\/local\/include \/usr\/include\/hdf5\/serial/g' Makefile.config && \
make all -j4 && make distribute

# Setup Intel-Caffe
export CAFFE_ROOT=${WORKDIR}/intel-caffe
export PYTHONPATH=${CAFFE_ROOT}/python
export PATH=$PATH:${CAFFE_ROOT}/distribute:${CAFFE_ROOT}/build/install:${CAFFE_ROOT}/build/install:${CAFFE_ROOT}/external/mkldnn/install
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${CAFFE_ROOT}/build/install/lib:${CAFFE_ROOT}/distribute/lib:${CAFFE_ROOT}/external/mkldnn/install/lib
export OPENBLAS_NUM_THREADS=`nproc`
export OMP_NUM_THREADS=`nproc`

# Build SAF
cd $WORKDIR
export CC="/usr/bin/gcc -std=c++11"
export CXX="/usr/bin/g++ -std=c++11"
git clone https://33f45c973b8b3d616a5b47c925c9142b3a09362a@github.com/syang100/saf.git && \
cd saf && \
# git reset --hard 9016203b218a5f1123ddca4d7ac46f3f81b1fdb1 && \
git reset --hard fb7b006df61b84e19ce2ab86d89cf8d22a0c98ca && \
mkdir -p ${WORKDIR}/saf/build && \
cd ${WORKDIR}/saf/build && \
cmake -DCMAKE_BUILD_TYPE=Release -DUSE_CAFFE=ON -DBACKEND=cpu -DUSE_SSD=ON -DUSE_WEBSOCKET=ON ..; make -j8 && make apps -j8

cd ${WORKDIR}/saf/config
cp config.toml.example config.toml && \
cp cameras.toml.example cameras.toml && \
cp models.toml.example models.toml

# Downgrade libmirprotobuf
echo "-Info: Downgrading libmirprotub3 to 0.21 version"
cd $WORKDIR
wget http://archive.ubuntu.com/ubuntu/pool/main/m/mir/libmirprotobuf3_0.21.0+16.04.20160330-0ubuntu1_amd64.deb && \
echo $sudoPW | sudo -S dpkg --force-downgrade -i libmirprotobuf3_0.21.0+16.04.20160330-0ubuntu1_amd64.deb && \
rm -f libmirprotobuf3_0.21.0+16.04.20160330-0ubuntu1_amd64.deb

# Download alexnet and data
echo "-Info: Download Caffe Alexnet Model & subset of Imagenet"
cd ${WORKDIR}/intel-caffe/scripts/; \
python ./download_model_binary.py ../models/bvlc_alexnet/; \
cd ${WORKDIR}/intel-caffe/data/ilsvrc12/; \
./get_ilsvrc_aux.sh;

cd ${WORKDIR}

cat <<EOF >> ~/.bashrc
export CAFFE_ROOT=${CAFFE_ROOT}
export PYTHONPATH=${PYTHONPATH}
export PATH=${PATH}
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}
export OPENBLAS_NUM_THREADS=${OPENBLAS_NUM_THREADS}
export OMP_NUM_THREADS=${OMP_NUM_THREADS}
EOF
