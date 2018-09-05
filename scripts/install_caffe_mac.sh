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

# Steps to compile Caffe on Mac
SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"
brew install -vd snappy leveldb gflags glog szip lmdb
brew install hdf5 opencv
brew install protobuf boost
brew install openblas
brew install python
brew install numpy

git clone https://github.com/BVLC/caffe
cd caffe
git reset --hard ${CAFFE_COMMIT_HASH}
# Patch Makefile.config to build only for Mac
patch Makefile.config.example < $SCRIPT_DIR/Caffe_Makefile.config.diff -o Makefile.config
# Patch Makefile to skip building Python interface
patch Makefile < $SCRIPT_DIR/Caffe_Makefile.diff
make -j4
sudo make distribute
cd ..
