#!/usr/bin/env python

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

import  safpy

sp =  safpy.GetInstance()

camera = sp.Camera('GST_TEST')
transformer1 = sp.Transformer('googlenet')
classifier1 = sp.Classifier('googlenet')
transformer2 = sp.Transformer('mobilenet')
classifier2 = sp.Classifier('mobilenet')

graph = { "camera" : [ "transformer1", "transformer2" ],
          "transformer1" : [ "classifier1" ],
          "transformer2" : [ "classifier2" ] }

sp.LoadGraph(graph)

sp.Start()

reader1 = sp.Subscribe(classifier1)
reader2 = sp.Subscribe(classifier2)

while True:
    try:
        fps1 = reader1.GetPushFps()
        fps2 = reader2.GetPushFps()
        print('classifier1 fps =', str(fps1))
        print('classifier2 fps =', str(fps2))
    except KeyboardInterrupt:
        break

sp.StopAndClean()
