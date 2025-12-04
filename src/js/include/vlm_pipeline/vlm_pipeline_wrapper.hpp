// Copyright (C) 2018-2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <thread>
#include <napi.h>
#include "openvino/genai/visual_language/pipeline.hpp"

class VLMPipelineWrapper : public Napi::ObjectWrap<VLMPipelineWrapper> {
public:
    VLMPipelineWrapper(const Napi::CallbackInfo& info);

    static Napi::Function get_class(Napi::Env env);

    Napi::Value init(const Napi::CallbackInfo& info);
    Napi::Value generate(const Napi::CallbackInfo& info);
    Napi::Value start_chat(const Napi::CallbackInfo& info);
    Napi::Value finish_chat(const Napi::CallbackInfo& info);
    Napi::Value get_tokenizer(const Napi::CallbackInfo& info);
    Napi::Value set_chat_template(const Napi::CallbackInfo& info);
    Napi::Value get_generation_config(const Napi::CallbackInfo& info);
    Napi::Value set_generation_config(const Napi::CallbackInfo& info);

private:
    bool is_loaded = false;
    bool is_initialized = false;
    bool is_running = false;

    std::string model_path;
    std::string device;

    std::shared_ptr<ov::genai::VLMPipeline> pipe = nullptr;
    std::function<bool(std::string)> streamer;
};
