// Copyright (C) 2018-2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "include/vlm_pipeline/init_worker.hpp"

VLMInitWorker::VLMInitWorker(
    Function& callback,
    std::shared_ptr<ov::genai::VLMPipeline>& pipe,
    const std::string model_path,
    const std::string device,
    const ov::AnyMap properties
) : AsyncWorker(callback), pipe(pipe), model_path(model_path), device(device), properties(properties) {};

void VLMInitWorker::Execute() {
    this->pipe = std::make_shared<ov::genai::VLMPipeline>(this->model_path, this->device, this->properties);
};

void VLMInitWorker::OnOK() {
    Callback().Call({ Env().Null() });
};
