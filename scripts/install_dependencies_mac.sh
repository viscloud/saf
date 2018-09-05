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

brew update
brew tap homebrew/science
brew install cmake glog glib gstreamer gst-plugins-base \
    gst-plugins-good gst-plugins-bad gst-plugins-ugly \
    boost opencv jemalloc

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"
# Install Caffe
INSTALL_DIR=$HOME/installed_${TRAVIS_OS_NAME}
if [ ! -d $INSTALL_DIR ]; then
    mkdir -p $INSTALL_DIR
fi

bash $SCRIPT_DIR/install_caffe_mac.sh
