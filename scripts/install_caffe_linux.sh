#!/usr/bin/env bash

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

# Install dependencies for Caffe
sudo apt-get -y install build-essential cmake git pkg-config libprotobuf-dev \
     libleveldb-dev libsnappy-dev libhdf5-serial-dev protobuf-compiler \
     libatlas-base-dev libgflags-dev libgoogle-glog-dev liblmdb-dev python-pip \
     python-dev python-numpy python-scipy
sudo apt-get -y install --no-install-recommends libboost-all-dev

# Set up the Caffe source code
git clone git@github.com:BVLC/caffe.git
cd caffe
git checkout 1.0

# Configure the build
cp Makefile.config.example Makefile.config
echo "OPENCV_VERSION := 3" >> Makefile.config
echo "CPU_ONLY := 1" >> Makefile.config

# Compile Caffe
make all
make test
make runtest
make distribute

cd ..
