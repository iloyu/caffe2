/**
 * Copyright (c) 2016-present, Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CAFFE2_OPERATORS_FULLY_CONNECTED_OP_H_
#define CAFFE2_OPERATORS_FULLY_CONNECTED_OP_H_

#include "caffe2/contrib/prof/prof_dag_net.h"
#include "caffe2/core/context.h"
#include "caffe2/core/operator.h"
#include "caffe2/utils/math.h"

namespace caffe2 {

// This operator outputs the ProfDAGNet stats
template <typename T, class Context, class Engine = DefaultEngine>
class GetProfDagStatsOp final : public Operator<Context> {
 public:
  USE_OPERATOR_CONTEXT_FUNCTIONS;
  GetProfDagStatsOp(const OperatorDef& operator_def, Workspace* ws)
      : Operator<Context>(operator_def, ws),
        net_name_(OperatorBase::GetSingleArgument<std::string>("net_name", "")),
        partial_net_name_(OperatorBase::GetSingleArgument<std::string>(
            "partial_net_name",
            "")),
        per_op_(OperatorBase::GetSingleArgument<bool>("per_op", false)) {
    ws_ = ws;
    CAFFE_ENFORCE(
        !(net_name_.empty() && partial_net_name_.empty()),
        "You need to provide net_name or partial_net_name");
    CAFFE_ENFORCE(
        net_name_.empty() || partial_net_name_.empty(),
        "You can not provide both net_name and partial_net_name");
  }
  ~GetProfDagStatsOp() {}

  bool RunOnDevice() override {
    // find the net by net_name_ or partial_net_name
    NetBase* net = nullptr;
    if (!net_name_.empty()) {
      net = ws_->GetNet(net_name_);
    } else if (!partial_net_name_.empty()) {
      for (auto& current_net : ws_->Nets()) {
        if (current_net.find(partial_net_name_) != std::string::npos) {
          CAFFE_ENFORCE(
              net == nullptr,
              "There are multiple nets with ",
              partial_net_name_,
              " as part of their name");
          net = ws_->GetNet(current_net);
        }
      }
      CAFFE_ENFORCE(
          net,
          "Can not find a net with ",
          partial_net_name_,
          " as part of its name");
    }

    auto prof_dag_net = dynamic_cast_if_rtti<ProfDAGNet*>(net);
    CAFFE_ENFORCE(prof_dag_net);

    ProfDAGProtos stats;
    if (per_op_) {
      stats = prof_dag_net->GetPerOperatorCost();
    } else {
      stats = prof_dag_net->GetOperatorStats();
    }

    // Write protobuf message to the output blob
    std::string serialized_data;
    CAFFE_ENFORCE(stats.SerializeToString(&serialized_data));
    Output(0)->Resize(1);
    Output(0)->template mutable_data<std::string>()[0] = serialized_data;

    return true;
  }

 protected:
  std::string net_name_;
  std::string partial_net_name_;
  bool per_op_;
  Workspace* ws_;
};

} // namespace caffe2

#endif // CAFFE2_OPERATORS_FULLY_CONNECTED_OP_H_
