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

#include "pipeline/pipeline.h"

#include <sstream>
#include <stdexcept>

#include <boost/graph/graphviz.hpp>
#include <boost/graph/topological_sort.hpp>

#include "common/types.h"
#include "operator/operator_factory.h"

constexpr auto DEFAULT_SINK_NAME = "output";

Pipeline::Pipeline() {}

std::shared_ptr<Pipeline> Pipeline::ConstructPipeline(nlohmann::json json) {
  nlohmann::json ops = json["operators"];

  auto pipeline = std::make_shared<Pipeline>();

  // First pass to create all operators
  for (const auto& op_spec : ops) {
    std::string op_name = op_spec["operator_name"];
    std::string op_type_str = op_spec["operator_type"];
    std::unordered_map<std::string, nlohmann::json> op_parameters_json =
        op_spec["parameters"]
            .get<std::unordered_map<std::string, nlohmann::json>>();
    std::unordered_map<std::string, std::string> op_parameters;
    for (const auto& pair : op_parameters_json) {
      std::string key = pair.first;
      std::string value = pair.second.get<std::string>();
      op_parameters[key] = value;
    }
    OperatorType op_type = GetOperatorTypeByString(op_type_str);
    FactoryParamsType params = (FactoryParamsType)op_parameters;

    LOG(INFO) << "Creating operator \"" << op_name << "\" of type \""
              << op_type_str << "\"";
    std::shared_ptr<Operator> op = OperatorFactory::Create(op_type, params);
    pipeline->ops_.insert({op_name, op});

    pipeline->op_names_.push_back(op_name);
    boost::add_vertex(op_name, pipeline->dependency_graph_);
    boost::add_vertex(op_name, pipeline->reverse_dependency_graph_);
  }

  // Second pass to create all operators
  for (const auto& op_spec : ops) {
    auto inputs_it = op_spec.find("inputs");
    if (inputs_it != op_spec.end()) {
      std::unordered_map<std::string, nlohmann::json> inputs =
          op_spec["inputs"]
              .get<std::unordered_map<std::string, nlohmann::json>>();

      std::string cur_op_id = op_spec["operator_name"];
      std::shared_ptr<Operator> cur_op = pipeline->GetOperator(cur_op_id);

      for (const auto& input : inputs) {
        std::string src = input.first;
        std::string stream_id = input.second;
        size_t i = stream_id.find(":");
        std::string src_op_id;
        std::string sink;
        if (i == std::string::npos) {
          // Use default sink name
          src_op_id = stream_id;
          sink = DEFAULT_SINK_NAME;
        } else {
          // Use custom sink name
          src_op_id = stream_id.substr(0, i);
          sink = stream_id.substr(i + 1, stream_id.length());
        }
        std::shared_ptr<Operator> src_op = pipeline->GetOperator(src_op_id);

        cur_op->SetSource(src, src_op->GetSink(sink));
        boost::add_edge_by_label(src_op_id, cur_op_id,
                                 pipeline->reverse_dependency_graph_);
        boost::add_edge_by_label(cur_op_id, src_op_id,
                                 pipeline->dependency_graph_);

        LOG(INFO) << "Connected source \"" << src << "\" of operator \""
                  << cur_op_id << "\" to the sink \"" << sink
                  << "\" from operator \"" << src_op_id << "\"";
      }
    }
  }

  return pipeline;
}

std::unordered_map<std::string, std::shared_ptr<Operator>>
Pipeline::GetOperators() {
  return ops_;
}

std::shared_ptr<Operator> Pipeline::GetOperator(const std::string& name) {
  if (ops_.find(name) == ops_.end()) {
    std::ostringstream msg;
    msg << "No Operator names \"" << name << "\"!";
    throw new std::invalid_argument(msg.str());
  }
  return ops_[name];
}

bool Pipeline::Start() {
  std::deque<Vertex> deque;
  boost::topological_sort(dependency_graph_, std::front_inserter(deque));

  std::ostringstream msg;
  msg << "Pipeline start order: ";
  for (const auto& i : deque) {
    std::string name = op_names_[i];
    msg << name << " ";
    if (!ops_[name]->Start()) {
      // If we were unable to start this Operator, then stop the pipeline and
      // return.
      Stop();
      return false;
    }
  }
  LOG(INFO) << msg.str();

  return true;
}

bool Pipeline::Stop() {
  std::deque<Vertex> deque;
  boost::topological_sort(reverse_dependency_graph_,
                          std::front_inserter(deque));

  std::ostringstream msg;
  msg << "Pipeline stop order: ";
  for (const auto& i : deque) {
    std::string name = op_names_[i];
    msg << name << " ";
    if (!ops_[name]->Stop()) {
      // This Operator refused to stop.
      return false;
    }
  }
  LOG(INFO) << msg.str();

  return true;
}

const std::string Pipeline::GetGraph() const {
  std::ostringstream o;
  boost::write_graphviz(o, reverse_dependency_graph_,
                        boost::make_label_writer(op_names_.data()));
  return o.str();
}
