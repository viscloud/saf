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

import cv2
import  safpy

sp =  safpy.GetInstance()

camera = sp.Camera('GST_TEST')
transformer = sp.Transformer('googlenet')
classifier = sp.Classifier('googlenet')

graph = { "camera" : [ "transformer" ],
          "transformer" : [ "classifier" ] }

sp.LoadGraph(graph)

sp.Start()

reader = sp.Subscribe(classifier)

while True:
    frame = reader.PopFrame()
    fps = reader.GetPushFps()
    image = frame.GetValue('original_image')
    cv2.putText(image, str(fps), (100,100), cv2.FONT_HERSHEY_SIMPLEX, 4, (255,255,255), 2, cv2.LINE_AA)
    cv2.imshow('saf', image)
    if cv2.waitKey(1) & 0xFF == ord('q'):
	break

cv2.destroyAllWindows()

sp.StopAndClean()
