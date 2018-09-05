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

opts, args = getopt.getopt(sys.argv[1:], 'h', ['sender_endpoint=', 'sender_package_type=', 'write_target=', 'write_uri=', 'help'])

sender_endpoint = 'ws://localhost:9002'
sender_package_type = 'frame'
write_target = 'file'
write_uri = 'sample.csv'

for key, value in opts:
    if key in ['-h', '--help']:
        print 'Writer programme'
        print '--sender_endpoint sender endpoint'
        print '--sender_package_type sender package type'
        print '--write_target write target'
        print '--write_uri write uri'
        sys.exit(0)
    if key in ['--sender_endpoint']:
        sender_endpoint = value
    if key in ['--sender_package_type']:
        sender_package_type = value
    if key in ['--write_target']:
        write_target = value
    if key in ['--write_uri']:
        write_uri = value

sp =  safpy.GetInstance()

receiver = sp.Receiver(sender_endpoint, sender_package_type)
writer = sp.Writer(write_target, write_uri)

sp.Add(receiver)
sp.Add(writer)

sp.Connect(receiver, writer)

sp.Start()

raw_input("Press Q to quit...\n")

sp.StopAndClean()
