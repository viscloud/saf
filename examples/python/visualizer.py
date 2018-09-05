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
import cv2
import getopt
import sys

opts, args = getopt.getopt(sys.argv[1:], 'h', ['sender_endpoint=', 'sender_package_type=', 'help'])

sender_endpoint = 'ws://localhost:9002'
sender_package_type = 'frame'

for key, value in opts:
    if key in ['-h', '--help']:
        print 'Visualizer programme'
        print '--sender_endpoint sender endpoint'
        print '--sender_package_type sender package type'
        sys.exit(0)
    if key in ['--sender_endpoint']:
        sender_endpoint = value
    if key in ['--sender_package_type']:
        sender_package_type = value

sp =  safpy.GetInstance()

receiver = sp.Receiver(sender_endpoint, sender_package_type)

sp.Add(receiver)

sp.Start()

sp.Visualize()

sp.StopAndClean()
