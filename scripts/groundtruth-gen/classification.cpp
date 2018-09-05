/*
All modifications made by the SAF Authors:
Copyright 2018 The SAF Authors. All rights reserved.

All modification made by Intel Corporation: © 2016 Intel Corporation

All contributions by the University of California:
Copyright (c) 2014, 2015, The Regents of the University of California (Regents)
All rights reserved.

All other contributions:
Copyright (c) 2014, 2015, the respective contributors
All rights reserved.
For the list of contributors go to
https://github.com/BVLC/caffe/blob/master/CONTRIBUTORS.md


Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <caffe/caffe.hpp>
#ifdef USE_OPENCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#endif  // USE_OPENCV
#include <algorithm>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

using namespace caffe;  // NOLINT(build/namespaces)
using std::string;

class Classifier {
 public:
  Classifier(const string& model_file, const string& trained_file,
             const string& engine);

  void Classify(const cv::Mat& img);

 private:
  void SetMean(void);

  void WrapInputLayer(std::vector<cv::Mat>* input_channels);

  void Preprocess(const cv::Mat& img, std::vector<cv::Mat>* input_channels);

 private:
  shared_ptr<Net<float>> net_;
  cv::Size input_geometry_;
  int num_channels_;
  cv::Mat mean_;
};

namespace boost {
namespace serialization {
template <class Archive>
void serialize(Archive& ar, cv::Mat& mat, const unsigned int) {
  int cols, rows, channels, type;

  if (Archive::is_saving::value) {
    rows = mat.size[0];
    cols = mat.size[1];
    channels = mat.size[2];
    type = mat.type();
  }

  ar& cols& rows& channels& type;

  if (Archive::is_loading::value) mat.create({rows, cols, channels}, type);
  const unsigned int row_size = cols * channels * mat.elemSize();
  for (int i = 0; i < rows; i++) {
    ar& boost::serialization::make_array(mat.ptr(i), row_size);
  }
}

}  // namespace serialization
}  // namespace boost

Classifier::Classifier(const string& model_file, const string& trained_file,
                       const string& engine) {
  // Force CPU mode
  Caffe::set_mode(Caffe::CPU);

  /* Load the network. */
  net_.reset(new Net<float>(model_file, TEST, 0, NULL, NULL, engine));
  net_->CopyTrainedLayersFrom(trained_file);

  CHECK_EQ(net_->num_inputs(), 1) << "Network should have exactly one input.";
  CHECK_EQ(net_->num_outputs(), 1) << "Network should have exactly one output.";

  Blob<float>* input_layer = net_->input_blobs()[0];
  num_channels_ = input_layer->channels();
  CHECK(num_channels_ == 3 || num_channels_ == 1)
      << "Input layer should have 1 or 3 channels.";
  input_geometry_ = cv::Size(input_layer->width(), input_layer->height());

  /* Load the binaryproto mean file. */
  SetMean();
}

/* Return the top N predictions. */
void Classifier::Classify(const cv::Mat& img) {
  Blob<float>* input_layer = net_->input_blobs()[0];
  input_layer->Reshape(1, num_channels_, input_geometry_.height,
                       input_geometry_.width);
  net_->Reshape();
  std::vector<cv::Mat> input_channels;
  WrapInputLayer(&input_channels);

  Preprocess(img, &input_channels);

  net_->Forward();
  std::vector<std::vector<caffe::Blob<float>*>> layer_activations =
      net_->top_vecs();
  std::vector<std::string> layer_names = net_->layer_names();
  CHECK(layer_activations.size() == layer_names.size());
  for (int i = 1; i < layer_activations.size(); ++i) {
    std::string cur_layer_name = layer_names.at(i);
    caffe::Blob<float>* cur_activations = layer_activations.at(i).at(0);
    int blob_dimensionality = cur_activations->num_axes();
    int batch_size = cur_activations->shape(0);
    CHECK(batch_size == 1);
    int num_channel = cur_activations->shape(1);
    int height = blob_dimensionality >= 3 ? cur_activations->shape(2) : 1;
    int width = blob_dimensionality >= 4 ? cur_activations->shape(3) : 1;
    // Convert format
    int per_channel_elems = height * width;
    cv::Mat activations({num_channel, height, width}, CV_32F);
    memcpy(activations.data, cur_activations->mutable_cpu_data(),
           num_channel * per_channel_elems * sizeof(float));
    /*
      std::vector<cv::Mat> channels;
      for(int ch = 0; ch < num_channel; ++ch) {
        cv::Mat cur_channel(height, width, CV_32F);
        memcpy(cur_channel.data, cur_activations->mutable_cpu_data() +
      per_channel_elems * ch, per_channel_elems * sizeof(float));
        channels.push_back(cur_channel);
      }
      cv::Mat activations({num_channel, height, width}, CV_32F);
      cv::merge(channels, activations);
    */
    std::cout << cur_layer_name << " <" << height << ", " << width << ", "
              << num_channel << "> " << std::endl;
    std::string filename = cur_layer_name + ".bin";
    std::replace(filename.begin(), filename.end(), '/', '.');
    std::ofstream of("activations/" + filename,
                     std::ios::binary | std::ios::out);
    boost::archive::binary_oarchive ar(of);
    ar << activations;
    of.close();
    // double check the things
    std::ifstream infile("activations/" + filename, std::ios::binary);
    boost::archive::binary_iarchive ir(infile);
    cv::Mat loaded_activations;
    ir >> loaded_activations;
    const unsigned int data_size = height * width * num_channel;
    float* truth = (float*)activations.ptr();
    float* loaded = (float*)loaded_activations.ptr();
    bool continuous = activations.isContinuous();
    continuous = continuous && loaded_activations.isContinuous();
    CHECK(continuous) << "Not continuous";
    for (int idx = 0; idx < data_size; ++idx) {
      CHECK(truth[idx] == loaded[idx])
          << cur_layer_name << ": " << idx << ": Expected " << truth[idx]
          << " found " << loaded[idx];
    }
  }
}

/* Load the mean file in binaryproto format. */
void Classifier::SetMean(void) {
  // Hardcoded to be these numbers
  mean_ = cv::Mat(input_geometry_, CV_32FC3, cv::Scalar(104.0, 117.0, 123.0));
}

/* Wrap the input layer of the network in separate cv::Mat objects
 * (one per channel). This way we save one memcpy operation and we
 * don't need to rely on cudaMemcpy2D. The last preprocessing
 * operation will write the separate channels directly to the input
 * layer. */
void Classifier::WrapInputLayer(std::vector<cv::Mat>* input_channels) {
  Blob<float>* input_layer = net_->input_blobs()[0];

  int width = input_layer->width();
  int height = input_layer->height();
  std::cout << width << " " << height << std::endl;
  float* input_data = input_layer->mutable_cpu_data();
  for (int i = 0; i < input_layer->channels(); ++i) {
    cv::Mat channel(height, width, CV_32FC1, input_data);
    input_channels->push_back(channel);
    input_data += width * height;
  }
}

void Classifier::Preprocess(const cv::Mat& img,
                            std::vector<cv::Mat>* input_channels) {
  /* Convert the input image to the input image format of the network. */
  cv::Mat sample;
  if (img.channels() == 3 && num_channels_ == 1)
    cv::cvtColor(img, sample, cv::COLOR_BGR2GRAY);
  else if (img.channels() == 4 && num_channels_ == 1)
    cv::cvtColor(img, sample, cv::COLOR_BGRA2GRAY);
  else if (img.channels() == 4 && num_channels_ == 3)
    cv::cvtColor(img, sample, cv::COLOR_BGRA2BGR);
  else if (img.channels() == 1 && num_channels_ == 3)
    cv::cvtColor(img, sample, cv::COLOR_GRAY2BGR);
  else
    sample = img;

  cv::Mat sample_resized;
  if (sample.size() != input_geometry_)
    cv::resize(sample, sample_resized, input_geometry_);
  else
    sample_resized = sample;

  cv::Mat sample_float;
  if (num_channels_ == 3)
    sample_resized.convertTo(sample_float, CV_32FC3);
  else
    sample_resized.convertTo(sample_float, CV_32FC1);

  cv::Mat sample_normalized;
  cv::subtract(sample_float, mean_, sample_normalized);

  /* This operation will write the separate BGR planes directly to the
   * input layer of the network because it is wrapped by the cv::Mat
   * objects in input_channels. */
  cv::split(sample_normalized, *input_channels);

  CHECK(reinterpret_cast<float*>(input_channels->at(0).data) ==
        net_->input_blobs()[0]->cpu_data())
      << "Input channels are not wrapping the input layer of the network.";
}

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " deploy.prototxt network.caffemodel"
              << " img.jpg [CAFFE|MKL2017|MKLDNN]" << std::endl;
    return 1;
  }

  ::google::InitGoogleLogging(argv[0]);

  string model_file = argv[1];
  string trained_file = argv[2];
  string file = argv[3];
  string engine = "";
  if (argc > 5) {
    engine = argv[5];
  }

  Classifier classifier(model_file, trained_file, engine);

  cv::Mat img = cv::imread(file, -1);
  CHECK(!img.empty()) << "Unable to decode image " << file;
  classifier.Classify(img);
}
