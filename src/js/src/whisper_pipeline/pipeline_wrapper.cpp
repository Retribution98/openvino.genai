// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "include/whisper_pipeline/pipeline_wrapper.hpp"

#include "include/helper.hpp"
#include "include/tokenizer.hpp"
#include "include/whisper_pipeline/generate_worker.hpp"
#include "include/whisper_pipeline/init_worker.hpp"

WhisperPipelineWrapper::WhisperPipelineWrapper(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<WhisperPipelineWrapper>(info) {}

Napi::Function WhisperPipelineWrapper::get_class(Napi::Env env) {
    return DefineClass(env,
                       "WhisperPipeline",
                       {
                           InstanceMethod("init", &WhisperPipelineWrapper::init),
                           InstanceMethod("generate", &WhisperPipelineWrapper::generate),
                           InstanceMethod("getTokenizer", &WhisperPipelineWrapper::get_tokenizer),
                           InstanceMethod("getGenerationConfig", &WhisperPipelineWrapper::get_generation_config),
                           InstanceMethod("setGenerationConfig", &WhisperPipelineWrapper::set_generation_config),
                       });
}

Napi::Value WhisperPipelineWrapper::init(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    try {
        OPENVINO_ASSERT(!this->pipe, "Pipeline is already initialized");
        OPENVINO_ASSERT(!*this->is_initializing, "Pipeline is already initializing");
        *this->is_initializing = true;

        VALIDATE_ARGS_COUNT(info, 4, "init()");
        auto model_path = js_to_cpp<std::string>(env, info[0]);
        auto device = js_to_cpp<std::string>(env, info[1]);
        auto properties = js_to_cpp<ov::AnyMap>(env, info[2]);
        OPENVINO_ASSERT(info[3].IsFunction(), "init callback is not a function");
        Napi::Function callback = info[3].As<Napi::Function>();

        auto async_worker = new WhisperInitWorker(callback,
                                                  this->pipe,
                                                  this->is_initializing,
                                                  std::move(model_path),
                                                  std::move(device),
                                                  std::move(properties));
        async_worker->Queue();
    } catch (const std::exception& ex) {
        *this->is_initializing = false;
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }

    return env.Undefined();
}

Napi::Value WhisperPipelineWrapper::generate(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    try {
        OPENVINO_ASSERT(this->pipe, "WhisperPipeline is not initialized");
        OPENVINO_ASSERT(!*this->is_generating, "Another generate is already in progress");
        *this->is_generating = true;

        OPENVINO_ASSERT(info.Length() >= 2 && info.Length() <= 4,
                       "generate() expects 2 (rawSpeech, callback), 3 (+ generationConfig), or 4 (+ streamer) arguments");
        auto raw_speech = js_to_cpp<std::vector<float>>(env, info[0]);
        ov::AnyMap generation_config;
        Napi::Function callback;
        std::optional<Napi::ThreadSafeFunction> streamer_tsfn = std::nullopt;
        if (info.Length() == 2) {
            OPENVINO_ASSERT(info[1].IsFunction(), "generate callback is not a function");
            callback = info[1].As<Napi::Function>();
        } else if (info.Length() == 3) {
            if (!info[1].IsUndefined() && !info[1].IsNull()) {
                generation_config = js_to_cpp<ov::AnyMap>(env, info[1]);
            }
            OPENVINO_ASSERT(info[2].IsFunction(), "generate callback is not a function");
            callback = info[2].As<Napi::Function>();
        } else {
            if (!info[1].IsUndefined() && !info[1].IsNull()) {
                generation_config = js_to_cpp<ov::AnyMap>(env, info[1]);
            }
            OPENVINO_ASSERT(info[2].IsFunction() || info[2].IsUndefined(), "streamer must be a function or undefined");
            OPENVINO_ASSERT(info[3].IsFunction(), "generate callback is not a function");
            callback = info[3].As<Napi::Function>();
            if (info[2].IsFunction()) {
                streamer_tsfn = Napi::ThreadSafeFunction::New(env,
                                                             info[2].As<Napi::Function>(),
                                                             "Whisper_generate_streamer",
                                                             0,
                                                             1);
            }
        }

        auto async_worker = new WhisperGenerateWorker(callback,
                                                     this->pipe,
                                                     this->is_generating,
                                                     std::move(raw_speech),
                                                     std::move(generation_config),
                                                     std::move(streamer_tsfn));
        async_worker->Queue();
    } catch (const std::exception& ex) {
        *this->is_generating = false;
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }

    return env.Undefined();
}

Napi::Value WhisperPipelineWrapper::get_tokenizer(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    try {
        OPENVINO_ASSERT(this->pipe, "WhisperPipeline is not initialized");
        auto tokenizer = this->pipe->get_tokenizer();
        return TokenizerWrapper::wrap(env, tokenizer);
    } catch (const std::exception& ex) {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value WhisperPipelineWrapper::get_generation_config(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    try {
        OPENVINO_ASSERT(this->pipe, "WhisperPipeline is not initialized");
        auto config = this->pipe->get_generation_config();
        Napi::Object obj = Napi::Object::New(env);
        if (config.language.has_value()) {
            obj.Set("language", Napi::String::New(env, config.language.value()));
        }
        if (config.task.has_value()) {
            obj.Set("task", Napi::String::New(env, config.task.value()));
        }
        obj.Set("return_timestamps", Napi::Boolean::New(env, config.return_timestamps));
        obj.Set("word_timestamps", Napi::Boolean::New(env, config.word_timestamps));
        obj.Set("max_new_tokens", Napi::Number::New(env, config.max_new_tokens));
        obj.Set("eos_token_id", Napi::Number::New(env, config.eos_token_id));
        return obj;
    } catch (const std::exception& ex) {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value WhisperPipelineWrapper::set_generation_config(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    try {
        OPENVINO_ASSERT(this->pipe, "WhisperPipeline is not initialized");
        VALIDATE_ARGS_COUNT(info, 1, "setGenerationConfig()");
        if (info[0].IsUndefined() || info[0].IsNull()) {
            return env.Undefined();
        }
        auto config_map = js_to_cpp<ov::AnyMap>(env, info[0]);
        auto config = this->pipe->get_generation_config();
        config.update_generation_config(config_map);
        this->pipe->set_generation_config(config);
    } catch (const std::exception& ex) {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}
