// Copyright (C) 2018-2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <napi.h>
#include "openvino/genai/visual_language/pipeline.hpp"

using namespace Napi;

class VLMInitWorker : public AsyncWorker {
public:
    VLMInitWorker(Function& callback,
                  std::shared_ptr<ov::genai::VLMPipeline>& pipe,
                  const std::string model_path,
                  std::string device,
                  ov::AnyMap properties);
    virtual ~VLMInitWorker() {}

    void Execute() override;
    void OnOK() override;

private:
    std::shared_ptr<ov::genai::VLMPipeline>& pipe;
    std::string model_path;
    std::string device;
    ov::AnyMap properties;
};
