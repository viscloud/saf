## Streaming Analytics Framework (SAF)
[![GitHub license](https://img.shields.io/badge/license-apache-green.svg?style=flat)](https://www.apache.org/licenses/LICENSE-2.0)

SAF is a platform for real-time video ingestion and analytics, primarily of
live camera streams, with a focus on running state-of-the-art machine
learning/computer vision algorithms. The goals of SAF are twofold:
* Deploy real-time, edge-to-cloud ML/CV applications across geo-distributed cameras.
* Provide an expressive pipeline-based programming model that makes it easy to define
complex applications that operate on many camera streams.

An example end-to-end analytics pipeline:
* Ingest a camera feed over RTSP.
* Decode the stream using a hardware H.264 decoder.
* Transform, scale, and normalize the frames into BGR images that can be fed
into a deep neural network.
* Run an image classification DNN (e.g. GoogleNet, MobileNet).
* Store the classification results and the original stream on disk.

SAF currently supports these DNN frameworks:
* [TensorFlow](https://github.com/tensorflow/tensorflow)
* [Intel Caffe](https://github.com/intel/caffe)
* [BVLC Caffe](https://github.com/BVLC/caffe)
* [OpenCL Caffe](https://github.com/BVLC/caffe/tree/opencl)

SAF is a collaboration between Carnegie Mellon University and Intel Corporation.

## Contact
Please file a [GitHub Issue](https://github.com/viscloud/saf/issues) if you experience difficulties compiling or running SAF, have a feature request, or would like additional information. We appreciate feedback and are constantly looking for more real-world use cases for SAF.

## Quick start
See [these detailed instructions](https://github.com/viscloud/saf/wiki/Building) for more information about
compiling SAF and its dependencies.

### 1. Compile SAF
```sh
git clone https://github.com/viscloud/saf
cd saf
mkdir build
cd build
cmake ..
make
```

### 2. Example: View a camera feed
The [`apps`](https://github.com/viscloud/saf/tree/master/apps) directory contains
several example SAF applications. The
[`simple`](https://github.com/viscloud/saf/blob/master/apps/simple/simple.cpp) app
displays frames from a camera on the screen. A few example invocations are included below.
See the `config/cameras.toml` file for more details on specifying cameras.
```sh
make simple
./apps/simple --display --camera GST_TEST
./apps/simple --display --camera webcam-linux
./apps/simple --display --camera webcam-mac
./apps/simple --display --camera file
```

**Note 1:** The `--display` option requires X11, but the `simple` app works without
it and will print some basic frame statistics to the terminal.

**Note 2:** By default, the `file` camera reads from the file `video.mp4` in the
current directory (see
[`config/cameras.toml`](https://github.com/viscloud/saf/blob/master/config/cameras.toml.example)).

### 3. Example: Classify frames using Caffe
The [`classifier`](https://github.com/viscloud/saf/blob/master/apps/classifier/classifier.cpp) app runs an image classification DNN on every frame and overlays the results in the viewer.

#### 3.1. Install Caffe
On Ubuntu, an unoptimized version of Caffe is available from `apt`. It requires a BLAS library, such as OpenBLAS.
```sh
sudo apt install -y libcaffe-cpu-dev libopenblas-dev
```

**Note:** If you are compiling Caffe from source, then see [this page](https://github.com/viscloud/saf/wiki/Building:-Caffe-Support) for more details.

#### 3.2. Enable Caffe support in SAF
We need to recompile SAF with Caffe support.
```sh
cmake -DUSE_CAFFE=yes ..
make
```

#### 3.3. Configure the models
Next, we need to configure our Caffe models by editing the default model configuration file.
```sh
cd <path to SAF>/config
cp models.toml.example models.toml
```

The default `models.toml` file has configurations for using Caffe to perform image
classification using GoogleNet and MobileNet. Note that some of the parameters
(`input_width`, `input_height`, and `default_output_layer`) are model specific.

The `models.toml` file contains the URLs for the various model files. There are
three files for each of the two models, but one of the files is shared--thus a
total of five files for both GoogleNet and MobileNet. Download the model files into a `models` directory:
```sh
mkdir <path to SAF>/models
cd <path to SAF>/models
# Download the model files as described in "models.toml"
wget ...
```

#### 3.4. Run the classification demo
```sh
cd <path to SAF>/build
make classifier
./apps/classifier --config-dir ../config --display --camera webcam-linux --model googlenet
./apps/classifier --config-dir ../config --display --camera webcam-linux --model mobilenet
```

The `classifier` app also works without the `--display` option.
