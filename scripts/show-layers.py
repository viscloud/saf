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

# The code in this file was heavily influenced by the TensorFlow example code
# and posts on Stack Overflow.

import sys

import numpy as np
import tensorflow as tf
import google
from google.protobuf import text_format

FLAGS = tf.app.flags.FLAGS;

def main():
  # TODO: Does python guarantee that these expressions are evaluated from left to right?
  if(len(sys.argv) < 2 or "-h" in sys.argv[1]):
    print("Prints the names and sizes of all tensors in graph");
    print("Usage: show-layers.py /path/to/model.pb");
    return;

  model_path = sys.argv[1];

  # Import graph

  with tf.gfile.GFile(model_path, "rb") as f:
    graph_def = tf.GraphDef();

    if(model_path[-5:] == "pbtxt"):
      graph_def = text_format.Parse(f.read(), tf.GraphDef())
    else:
      graph_def.ParseFromString(f.read());
    tf.import_graph_def(graph_def, name="", input_map=None, producer_op_list=None, op_dict=None, return_elements=None);

  with tf.Session() as sess:
    for op in sess.graph.get_operations():
      try:
        tensor = tf.Graph.get_tensor_by_name(tf.get_default_graph(), op.name + ":0");
      except:
        continue
      print("NAME = \"" + tensor.name + "\"  SHAPE = " + str(tensor.get_shape()));
    return;

main();
