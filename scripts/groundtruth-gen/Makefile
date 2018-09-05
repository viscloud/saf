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

all:
	g++ classification.cpp -fPIE -fstack-protector -fPIC -fno-operator-names -Wformat -Wformat-security -Wall -D_FORTIFY_SOURCE=2 -Wno-unused-local-typedefs -DMKL2017_SUPPORTED -DUSE_MKL2017_AS_DEFAULT_ENGINE -MMD -MP -std=c++11 -pthread -fPIC -DCAFFE_VERSION=1.0.0-rc3 -DNDEBUG -O3 -DUSE_OPENCV -DUSE_LEVELDB -DUSE_LMDB -DCPU_ONLY -DUSE_MKL -I/usr/include/python2.7 -I/usr/lib/python2.7/dist-packages/numpy/core/include -I/usr/local/include -I/usr/include/hdf5/serial/ -I../.build_release/src -I../src -I../include -I../src/gtest/include -I../src/gtest/ -I../src/gmock/include -I../src/gmock/ -I/home/tskim/src/caffe/./external/mkl/mklml_lnx_2018.0.20170720//include -I/home/tskim/src/caffe/external/mkldnn/install/include -Wall -Wno-sign-compare -fopenmp -DMKLDNN_SUPPORTED -c -o classification.o
	g++ classification.o -o classification.bin -lboost_system -lboost_filesystem -lboost_regex -lboost_serialization -pthread -fPIC -DCAFFE_VERSION=1.0.0-rc3 -DNDEBUG -O3 -DUSE_OPENCV -DUSE_LEVELDB -DUSE_LMDB -DCPU_ONLY -DUSE_MKL -I/usr/include/python2.7 -I/usr/lib/python2.7/dist-packages/numpy/core/include -I/usr/local/include -I/usr/include/hdf5/serial/ -I../.build_release/src -I../src -I../include -I../src/gtest/include -I../src/gtest/ -I../src/gmock/include -I../src/gmock/ -I/home/tskim/src/caffe/./external/mkl/mklml_lnx_2018.0.20170720//include -I/home/tskim/src/caffe/external/mkldnn/install/include -Wall -Wno-sign-compare -fopenmp -lmklml_intel -Wl,-rpath,/home/tskim/src/caffe/./external/mkl/mklml_lnx_2018.0.20170720//lib -lmkldnn -L/home/tskim/src/caffe/external/mkldnn/install/lib -Wl,-rpath,/home/tskim/src/caffe/external/mkldnn/install/lib -fPIE -fstack-protector -fPIC -fno-operator-names -Wformat -Wformat-security -Wall -D_FORTIFY_SOURCE=2 -z noexecstack -z relro -z now -pie -lcaffe -L/usr/lib -L/usr/local/lib -L/usr/lib -L/home/tskim/src/caffe/./external/mkl/mklml_lnx_2018.0.20170720//lib -L/home/tskim/src/caffe/./external/mkl/mklml_lnx_2018.0.20170720//lib/intel64 -L../.build_release/lib -L/home/tskim/src/caffe/./external/mkl/mklml_lnx_2018.0.20170720/lib  -lglog -lgflags -lprotobuf -lm -lhdf5_serial_hl -lhdf5_serial -lleveldb -lsnappy -llmdb -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs -lopencv_videoio -lboost_thread -lstdc++ -liomp5 -Wl,--as-needed
clean:
	rm -rf *.bin *.o *.d activations/*
run:
	LD_LIBRARY_PATH=../distribute/lib/ ./classification.bin
