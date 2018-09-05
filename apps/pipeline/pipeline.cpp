// Copyright 2018 The SAF Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Deploys a pipeline from a JSON specification

#include <atomic>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <boost/program_options.hpp>
#ifdef GRAPHVIZ
#include <graphviz/gvc.h>
#endif  // GRAPHVIZ
#include <json/src/json.hpp>
#include <opencv2/opencv.hpp>

#include "saf.h"

namespace po = boost::program_options;

#ifdef GRAPHVIZ
static std::atomic<bool> stopped(false);

static std::shared_ptr<std::thread> ShowGraph(const std::string& graph) {
  Agraph_t* dg = agmemread(graph.c_str());
  CHECK(dg != NULL);

  GVC_t* gvc = gvContext();
  int err = gvLayout(gvc, dg, "dot");
  CHECK(err == 0);

  char* buf;
  unsigned int len;
  err = gvRenderData(gvc, dg, "bmp", &buf, &len);
  CHECK(err == 0);
  buf = (char*)realloc(buf, len + 1);

  std::vector<char> data(buf, buf + len);
  gvFreeRenderData(buf);

  cv::Mat data_mat(data, true);
  cv::Mat img = cv::imdecode(data_mat, cv::IMREAD_COLOR);

  auto t = std::make_shared<std::thread>([img] {
    while (!stopped) {
      cv::imshow("Pipeline Graph", img);
      cv::waitKey(10);
    }
  });

  return t;
}
#endif  // GRAPHVIZ

void Run(const std::string& pipeline_filepath, bool dry_run, bool show_graph,
         bool dump_graph, const std::string& dump_graph_filepath) {
  std::ifstream i(pipeline_filepath);
  nlohmann::json json;
  i >> json;
  std::shared_ptr<Pipeline> pipeline = Pipeline::ConstructPipeline(json);

  // Generate the graph representation of the pipeline.
  std::string graph = pipeline->GetGraph();
  if (dump_graph) {
    std::ofstream graph_file(dump_graph_filepath,
                             std::ofstream::out | std::ofstream::trunc);
    graph_file << graph;
    graph_file.close();
  }

  std::shared_ptr<std::thread> graph_thread = nullptr;
  if (show_graph) {
#ifdef GRAPHVIZ
    graph_thread = ShowGraph(graph);
#else
    throw std::runtime_error(
        "Please install the \"libgraphviz-dev\" package and "
        "recompile this app to enable displaying the pipeline graph.");
#endif  // GRAPHVIZ
  }

  if (!dry_run) {
    pipeline->Start();
  }

  if (!dry_run || show_graph) {
    // We wait for "Enter" if we are actually running the pipeline or if we are
    // displaying the pipeline graph.
    std::cout << "Press \"Enter\" to stop." << std::endl;
    getchar();
  }

#ifdef GRAPHVIZ
  if (show_graph) {
    stopped = true;
    graph_thread->join();
  }
#endif  // GRAPHVIZ

  if (!dry_run) {
    pipeline->Stop();
  }
}

int main(int argc, char* argv[]) {
  po::options_description desc("Runs a pipeline described by a JSON file");
  desc.add_options()("help,h", "print the help message");
  desc.add_options()("config-dir,C", po::value<std::string>(),
                     "The directory containing SAF's config files.");
  desc.add_options()("pipeline,p", po::value<std::string>()->required(),
                     "Path to a JSON file describing a pipeline.");
  desc.add_options()("dry-run,n", "Create the pipeline but do not run it.");
  desc.add_options()("graph,g",
                     "Display the pipeline graph. Requires the "
                     "\"libgraphviz-dev\" package.");
  desc.add_options()("dump-graph,o", po::value<std::string>(),
                     "Save the GraphViz textual representation in "
                     "the specified file.");

  // Parse the command line arguments.
  po::variables_map args;
  try {
    po::store(po::parse_command_line(argc, argv, desc), args);
    if (args.count("help")) {
      std::cout << desc << std::endl;
      return 1;
    }
    po::notify(args);
  } catch (const po::error& e) {
    std::cerr << e.what() << std::endl;
    std::cout << desc << std::endl;
    return 1;
  }

  // Set up GStreamer.
  gst_init(&argc, &argv);
  // Set up glog.
  google::InitGoogleLogging(argv[0]);
  FLAGS_alsologtostderr = 1;
  FLAGS_colorlogtostderr = 1;

  // Extract the command line arguments.
  if (args.count("config-dir")) {
    Context::GetContext().SetConfigDir(args["config-dir"].as<std::string>());
  }
  // Initialize the SAF context. This must be called before using SAF.
  Context::GetContext().Init();

  std::string pipeline_filepath = args["pipeline"].as<std::string>();
  bool dry_run = args.count("dry-run");
  bool show_graph = args.count("graph");
  bool dump_graph = args.count("dump-graph");
  std::string dump_graph_filepath = "";
  if (dump_graph) {
    dump_graph_filepath = args["dump-graph"].as<std::string>();
  }
  Run(pipeline_filepath, dry_run, show_graph, dump_graph, dump_graph_filepath);
  return 0;
}
