/* Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include <cmath>
#include <iostream>
#include "../test_include.h"
#include "operators/activation_op.h"

namespace paddle_mobile {

void LeakyRelu(const framework::Tensor *X, framework::Tensor *Y, float alpha) {
  const float *x = X->data<float>();
  float *y = Y->mutable_data<float>();

  for (int i = 0; i < X->numel(); ++i) {
    y[i] = std::max(x[i], x[i] * alpha);
  }
}

int TestLeakyReluOp(const std::vector<int> input_shape, float alpha) {
  framework::DDim dims = framework::make_ddim(input_shape);
  VariableNameMap inputs;
  VariableNameMap outputs;
  auto scope = std::make_shared<framework::Scope>();
  inputs["X"] = std::vector<std::string>({"input"});
  outputs["Out"] = std::vector<std::string>({"output"});
  auto input_var = scope.get()->Var("input");
  auto input = input_var->template GetMutable<framework::LoDTensor>();
  SetupTensor<float>(input, dims, -100.0, 100.0);
  auto output_var = scope.get()->Var("output");
  framework::AttributeMap attrs;
  attrs["alpha"].Set<float>(alpha);

  auto *op = new operators::LeakyReluOp<CPU, float>(
      "leaky_relu", inputs, outputs, attrs, scope.get());
  op->InferShape();
  op->Init();
  op->Run();

  auto output = output_var->template Get<framework::LoDTensor>();

  framework::Tensor output_cmp;
  float *output_cmp_data = output_cmp.mutable_data<float>(output->dims());
  LeakyRelu(input, &output_cmp, alpha);

  const float *output_data = output->data<float>();
  for (int i = 0; i < output->numel(); ++i) {
    float gap = output_data[i] - output_cmp_data[i];
    if (gap > 1e-5 && std::abs(gap / (output_data[i] + 1e-5)) > 1e-3) {
      LOG(kLOG_INFO) << "output_data[" << i << "] = " << output_data[i]
                     << ", output_cmp_data[" << i
                     << "] = " << output_cmp_data[i];
      delete op;
      exit(1);
    }
  }
  delete op;
  return 0;
}

}  // namespace paddle_mobile

int main() {
  paddle_mobile::TestLeakyReluOp({1, 1, 2, 3}, 0.2f);
  paddle_mobile::TestLeakyReluOp({1, 3, 11, 22}, 0.3f);
  paddle_mobile::TestLeakyReluOp({1, 32, 112, 112}, 0.4f);
  std::cout << "test leaky_relu op pass." << std::endl;
  return 0;
}
