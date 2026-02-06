// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <napi.h>

#include <optional>

#include "openvino/genai/whisper_pipeline.hpp"

class WhisperGenerateWorker : public Napi::AsyncWorker {
public:
    WhisperGenerateWorker(Napi::Function& callback,
                         std::shared_ptr<ov::genai::WhisperPipeline> pipe,
                         std::shared_ptr<bool> is_generating,
                         std::vector<float>&& raw_speech,
                         ov::AnyMap&& generation_config,
                         std::optional<Napi::ThreadSafeFunction> streamer_tsfn = std::nullopt);
    virtual ~WhisperGenerateWorker() {}
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;

private:
    std::shared_ptr<ov::genai::WhisperPipeline> pipe;
    std::shared_ptr<bool> is_generating;
    std::vector<float> raw_speech;
    ov::AnyMap generation_config;
    std::optional<Napi::ThreadSafeFunction> streamer_tsfn;
    ov::genai::WhisperDecodedResults result;
};
