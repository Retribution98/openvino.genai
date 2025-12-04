// Copyright (C) 2018-2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "include/vlm_pipeline/vlm_perf_metrics.hpp"

#include "bindings_utils.hpp"
#include "include/addon.hpp"
#include "include/helper.hpp"

using ov::genai::common_bindings::utils::get_ms;
using ov::genai::common_bindings::utils::timestamp_to_ms;

VLMPerfMetricsWrapper::VLMPerfMetricsWrapper(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<VLMPerfMetricsWrapper>(info),
      _metrics{} {};

Napi::Function VLMPerfMetricsWrapper::get_class(Napi::Env env) {
    return DefineClass(
        env,
        "VLMPerfMetrics",
        {
            InstanceMethod("getLoadTime", &VLMPerfMetricsWrapper::get_load_time),
            InstanceMethod("getNumGeneratedTokens", &VLMPerfMetricsWrapper::get_num_generated_tokens),
            InstanceMethod("getNumInputTokens", &VLMPerfMetricsWrapper::get_num_input_tokens),
            InstanceMethod("getTTFT", &VLMPerfMetricsWrapper::get_ttft),
            InstanceMethod("getTPOT", &VLMPerfMetricsWrapper::get_tpot),
            InstanceMethod("getIPOT", &VLMPerfMetricsWrapper::get_ipot),
            InstanceMethod("getThroughput", &VLMPerfMetricsWrapper::get_throughput),
            InstanceMethod("getInferenceDuration", &VLMPerfMetricsWrapper::get_inference_duration),
            InstanceMethod("getGenerateDuration", &VLMPerfMetricsWrapper::get_generate_duration),
            InstanceMethod("getTokenizationDuration", &VLMPerfMetricsWrapper::get_tokenization_duration),
            InstanceMethod("getDetokenizationDuration", &VLMPerfMetricsWrapper::get_detokenization_duration),
            InstanceMethod("getGrammarCompilerInitTimes", &VLMPerfMetricsWrapper::get_grammar_compiler_init_times),
            InstanceMethod("getGrammarCompileTime", &VLMPerfMetricsWrapper::get_grammar_compile_time),
            InstanceMethod("getPrepareEmbeddingsDuration", &VLMPerfMetricsWrapper::get_prepare_embeddings_duration),
            InstanceAccessor<&VLMPerfMetricsWrapper::get_raw_metrics>("rawMetrics"),
            InstanceAccessor<&VLMPerfMetricsWrapper::get_vlm_raw_metrics>("vlmRawMetrics"),
            InstanceMethod("add", &VLMPerfMetricsWrapper::add),
        });
}

Napi::Object VLMPerfMetricsWrapper::wrap(Napi::Env env, const ov::genai::VLMPerfMetrics& metrics) {
    const auto& prototype = env.GetInstanceData<AddonData>()->vlm_perf_metrics;
    OPENVINO_ASSERT(prototype, "Invalid pointer to prototype.");
    auto obj = prototype.New({});
    const auto m_ptr = Napi::ObjectWrap<VLMPerfMetricsWrapper>::Unwrap(obj);
    m_ptr->_metrics = metrics;
    return obj;
}

Napi::Object create_mean_std_pair(Napi::Env env, const ov::genai::MeanStdPair& pair);
Napi::Object create_summary_stats(Napi::Env env, const ov::genai::SummaryStats& stats);

Napi::Value VLMPerfMetricsWrapper::get_load_time(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 0, "getLoadTime()");
    return Napi::Number::New(info.Env(), _metrics.get_load_time());
}

Napi::Value VLMPerfMetricsWrapper::get_num_generated_tokens(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 0, "getNumGeneratedTokens()");
    return Napi::Number::New(info.Env(), _metrics.get_num_generated_tokens());
}

Napi::Value VLMPerfMetricsWrapper::get_num_input_tokens(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 0, "getNumInputTokens()");
    return Napi::Number::New(info.Env(), _metrics.get_num_input_tokens());
}

Napi::Value VLMPerfMetricsWrapper::get_ttft(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 0, "getTTFT()");
    return create_mean_std_pair(info.Env(), _metrics.get_ttft());
}

Napi::Value VLMPerfMetricsWrapper::get_tpot(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 0, "getTPOT()");
    return create_mean_std_pair(info.Env(), _metrics.get_tpot());
}

Napi::Value VLMPerfMetricsWrapper::get_ipot(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 0, "getIPOT()");
    return create_mean_std_pair(info.Env(), _metrics.get_ipot());
}

Napi::Value VLMPerfMetricsWrapper::get_throughput(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 0, "getThroughput()");
    return create_mean_std_pair(info.Env(), _metrics.get_throughput());
}

Napi::Value VLMPerfMetricsWrapper::get_inference_duration(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 0, "getInferenceDuration()");
    return create_mean_std_pair(info.Env(), _metrics.get_inference_duration());
}

Napi::Value VLMPerfMetricsWrapper::get_generate_duration(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 0, "getGenerateDuration()");
    return create_mean_std_pair(info.Env(), _metrics.get_generate_duration());
}

Napi::Value VLMPerfMetricsWrapper::get_tokenization_duration(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 0, "getTokenizationDuration()");
    return create_mean_std_pair(info.Env(), _metrics.get_tokenization_duration());
}

Napi::Value VLMPerfMetricsWrapper::get_grammar_compiler_init_times(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 0, "getGrammarCompilerInitTimes()");
    return cpp_map_to_js_object(info.Env(), _metrics.get_grammar_compiler_init_times());
}

Napi::Value VLMPerfMetricsWrapper::get_grammar_compile_time(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 0, "getGrammarCompileTime()");
    return create_summary_stats(info.Env(), _metrics.get_grammar_compile_time());
};

Napi::Value VLMPerfMetricsWrapper::get_detokenization_duration(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 0, "getDetokenizationDuration()");
    return create_mean_std_pair(info.Env(), _metrics.get_detokenization_duration());
}

Napi::Value VLMPerfMetricsWrapper::get_prepare_embeddings_duration(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 0, "getPrepareEmbeddingsDuration()");
    return create_mean_std_pair(info.Env(), _metrics.get_prepare_embeddings_duration());
}

Napi::Value VLMPerfMetricsWrapper::get_raw_metrics(const Napi::CallbackInfo& info) {
    Napi::Object obj = Napi::Object::New(info.Env());
    obj.Set("generateDurations",
            cpp_to_js<std::vector<float>, Napi::Value>(
                info.Env(),
                get_ms(_metrics.raw_metrics, &ov::genai::RawPerfMetrics::generate_durations)));
    obj.Set("tokenizationDurations",
            cpp_to_js<std::vector<float>, Napi::Value>(
                info.Env(),
                get_ms(_metrics.raw_metrics, &ov::genai::RawPerfMetrics::tokenization_durations)));
    obj.Set("detokenizationDurations",
            cpp_to_js<std::vector<float>, Napi::Value>(
                info.Env(),
                get_ms(_metrics.raw_metrics, &ov::genai::RawPerfMetrics::detokenization_durations)));

    obj.Set("timesToFirstToken",
            cpp_to_js<std::vector<float>, Napi::Value>(
                info.Env(),
                get_ms(_metrics.raw_metrics, &ov::genai::RawPerfMetrics::m_times_to_first_token)));
    obj.Set("newTokenTimes",
            cpp_to_js<std::vector<double>, Napi::Value>(
                info.Env(),
                timestamp_to_ms(_metrics.raw_metrics, &ov::genai::RawPerfMetrics::m_new_token_times)));
    obj.Set("tokenInferDurations",
            cpp_to_js<std::vector<float>, Napi::Value>(
                info.Env(),
                get_ms(_metrics.raw_metrics, &ov::genai::RawPerfMetrics::m_token_infer_durations)));
    obj.Set("batchSizes", cpp_to_js<std::vector<size_t>, Napi::Value>(info.Env(), _metrics.raw_metrics.m_batch_sizes));
    obj.Set("durations",
            cpp_to_js<std::vector<float>, Napi::Value>(
                info.Env(),
                get_ms(_metrics.raw_metrics, &ov::genai::RawPerfMetrics::m_durations)));
    obj.Set("inferenceDurations",
            cpp_to_js<std::vector<float>, Napi::Value>(
                info.Env(),
                get_ms(_metrics.raw_metrics, &ov::genai::RawPerfMetrics::m_inference_durations)));

    obj.Set("grammarCompileTimes",
            cpp_to_js<std::vector<float>, Napi::Value>(
                info.Env(),
                get_ms(_metrics.raw_metrics, &ov::genai::RawPerfMetrics::m_grammar_compile_times)));

    return obj;
}

Napi::Value VLMPerfMetricsWrapper::get_vlm_raw_metrics(const Napi::CallbackInfo& info) {
    Napi::Object obj = Napi::Object::New(info.Env());
    obj.Set("prepareEmbeddingsDurations",
            cpp_to_js<std::vector<float>, Napi::Value>(
                info.Env(),
                get_ms(_metrics.vlm_raw_metrics, &ov::genai::VLMRawPerfMetrics::prepare_embeddings_durations)));

    return obj;
}

Napi::Value VLMPerfMetricsWrapper::add(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 1, "add()");
    const auto env = info.Env();
    try {
        const auto obj = info[0].As<Napi::Object>();
        const auto& prototype = env.GetInstanceData<AddonData>()->vlm_perf_metrics;

        OPENVINO_ASSERT(prototype, "Invalid pointer to prototype.");
        OPENVINO_ASSERT(obj.InstanceOf(prototype.Value().As<Napi::Function>()),
                        "Passed argument is not of type VLMPerfMetrics");

        const auto js_metrics = Napi::ObjectWrap<VLMPerfMetricsWrapper>::Unwrap(obj);
        _metrics = _metrics + js_metrics->get_value();
    } catch (const std::exception& ex) {
        Napi::TypeError::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return info.This();
}

ov::genai::VLMPerfMetrics& VLMPerfMetricsWrapper::get_value() {
    return _metrics;
}
