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
import time

sp =  safpy.GetInstance()

camera = sp.Camera('GST_TEST')
targets = ['person']
detector = sp.Detector('cvsdk-ssd', 'person-detection-retail-0012', 0.1, 2, 40, targets)
tracker = sp.Tracker('dlib')

writer = sp.Writer('file', 'testWriter.txt')

sp.Add(camera)
sp.Add(detector)
sp.Add(tracker)
sp.Add(writer)
sp.Connect(camera, detector, "input0", "output")
sp.Connect(detector, tracker, "input", "output0")
sp.Connect(tracker, writer, "input0", "output")

sp.Start()

time.sleep(10) # Write results for 10 seconds.

sp.StopAndClean()
