// Copyright (c) 2020 PaddlePaddle Authors. All Rights Reserved.
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

#include <gtest/gtest.h>
#include "lite/api/paddle_use_kernels.h"
#include "lite/api/paddle_use_ops.h"
#include "lite/core/arena/framework.h"

namespace paddle {
namespace lite {

class SequenceConvComputeTester : public arena::TestCase {
 public:
  SequenceConvComputeTester(const Place& place,
                            const std::string& alias,
                            LoD lod,
                            DDim dims,
                            const int& contextStart,
                            const int& contextStride,
                            const int& contextLength,
                            const int& kernel_num)
      : TestCase(place, alias),
        lod_(lod),
        dims_(dims),
        contextStart_(contextStart),
        contextStride_(contextStride),
        contextLength_(contextLength),
        kernel_num_(kernel_num) {}

  void PrepareOpDesc(cpp::OpDesc* op_desc) {
    op_desc->SetType("sequence_conv");
    op_desc->SetInput("X", {input_name_});
    op_desc->SetInput("Filter", {filter_name_});
    op_desc->SetOutput("Out", {output_name_});
    op_desc->SetAttr("contextStart", contextStart_);
    op_desc->SetAttr("contextStride", contextStride_);
    op_desc->SetAttr("contextLength", contextLength_);
  }

  void PrepareData() override {
    std::vector<float> din(dims_.production());
    for (int i = 0; i < dims_[0]; i++) {
      for (int j = 0; j < dims_[1]; j++) {
        din[i * dims_[1] + j] =
            (2.0 * i + 3.0 * j) / (2.0 * dims_[0] + 3.0 * dims_[1]) - 0.5;
      }
    }
    SetCommonTensor(input_name_, dims_, din.data(), lod_);

    DDim filter_dims(
        std::vector<int64_t>{contextLength_ * dims_[1], kernel_num_});
    std::vector<float> dfilter(filter_dims.production());
    for (int i = 0; i < filter_dims[0]; i++) {
      for (int j = 0; j < filter_dims[1]; j++) {
        dfilter[i * filter_dims[0] + j] =
            (1.5 * i + 2.0 * j) / (1.5 * filter_dims[0] + 2 * filter_dims[1]) -
            0.5;
      }
    }
    SetCommonTensor(filter_name_, filter_dims, dfilter.data(), lod_);
  }

  void RunBaseline(Scope* scope) override {
    // calculate res the output in this scope
    // to compare with the Paddle-Lite calculated one

    auto* output = scope->NewTensor(output_name_);
    CHECK(output);
    std::vector<int64_t> output_shape({4, 3});
    output->Resize(DDim(output_shape));
    auto output_dims = output->dims();
    auto output_data = output->mutable_data<float>();
    std::vector<std::vector<float>> res({{0.194508, 0.05720823, -0.08009153},
                                         {0.73512584, 0.5749428, 0.41475973},
                                         {0.5635012, 0.49485126, 0.42620137},
                                         {0.2517162, 0.23646072, 0.22120519}});

    for (int i = 0; i < output_shape[0]; i++) {
      for (int j = 0; j < output_shape[1]; j++) {
        output_data[i * output_shape[0] + j] = res[i][j];
      }
    }
  }

 protected:
  std::string input_name_ = "x";
  std::string filter_name_ = "filter";
  std::string output_name_ = "out";
  LoD lod_;
  DDim dims_;
  int contextStart_;
  int contextStride_;
  int contextLength_;
  int kernel_num_;
};

void TestNormalCase(Place place, float abs_error = 2e-5) {
  std::vector<std::vector<uint64_t>> lod{{0, 4}};
  std::vector<int64_t> dims({4, 5});
  std::unique_ptr<arena::TestCase> tester(new SequenceConvComputeTester(
      place, "def", lod, DDim(dims), -1, 1, 3, 3));
  arena::Arena arena(std::move(tester), place, abs_error);
  arena.TestPrecision();
}

TEST(sequence_conv, precision) {
#ifdef LITE_WITH_ARM
  float abs_error = 2e-5;
  Place place(TARGET(kARM));

  TestNormalCase(place, abs_error);
#endif
}

}  // namespace lite
}  // namespace paddle
