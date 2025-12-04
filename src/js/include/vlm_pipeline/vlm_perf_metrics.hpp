// Copyright (C) 2018-2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <napi.h>

#include "openvino/genai/visual_language/perf_metrics.hpp"

class VLMPerfMetricsWrapper : public Napi::ObjectWrap<VLMPerfMetricsWrapper> {
public:
    VLMPerfMetricsWrapper(const Napi::CallbackInfo& info);

    static Napi::Function get_class(Napi::Env env);
    static Napi::Object wrap(Napi::Env env, const ov::genai::VLMPerfMetrics& metrics);

    Napi::Value get_load_time(const Napi::CallbackInfo& info);
    Napi::Value get_num_generated_tokens(const Napi::CallbackInfo& info);
    Napi::Value get_num_input_tokens(const Napi::CallbackInfo& info);
    Napi::Value get_ttft(const Napi::CallbackInfo& info);
    Napi::Value get_tpot(const Napi::CallbackInfo& info);
    Napi::Value get_ipot(const Napi::CallbackInfo& info);
    Napi::Value get_throughput(const Napi::CallbackInfo& info);

    Napi::Value get_inference_duration(const Napi::CallbackInfo& info);
    Napi::Value get_generate_duration(const Napi::CallbackInfo& info);
    Napi::Value get_tokenization_duration(const Napi::CallbackInfo& info);
    Napi::Value get_detokenization_duration(const Napi::CallbackInfo& info);

    Napi::Value get_grammar_compiler_init_times(const Napi::CallbackInfo& info);
    Napi::Value get_grammar_compile_time(const Napi::CallbackInfo& info);

    Napi::Value get_prepare_embeddings_duration(const Napi::CallbackInfo& info);

    Napi::Value get_raw_metrics(const Napi::CallbackInfo& info);
    Napi::Value get_vlm_raw_metrics(const Napi::CallbackInfo& info);
    Napi::Value add(const Napi::CallbackInfo& info);
    ov::genai::VLMPerfMetrics& get_value();

private:
    ov::genai::VLMPerfMetrics _metrics;
};
