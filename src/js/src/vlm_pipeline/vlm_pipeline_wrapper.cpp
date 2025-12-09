// Copyright (C) 2018-2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "include/vlm_pipeline/vlm_pipeline_wrapper.hpp"

#include <future>
#include "include/addon.hpp"
#include "include/helper.hpp"
#include "include/vlm_pipeline/perf_metrics.hpp"
#include "include/vlm_pipeline/start_chat_worker.hpp"
#include "include/vlm_pipeline/finish_chat_worker.hpp"
#include "include/vlm_pipeline/init_worker.hpp"
#include "include/tokenizer.hpp"

struct VLMTsfnContext {
    VLMTsfnContext(std::string prompt) : prompt(prompt) {};
    ~VLMTsfnContext() {};

    std::thread native_thread;
    Napi::ThreadSafeFunction tsfn;

    std::string prompt;
    std::vector<ov::Tensor> images;
    std::vector<ov::Tensor> videos;
    std::shared_ptr<ov::genai::VLMPipeline> pipe = nullptr;
    std::shared_ptr<ov::AnyMap> generation_config = nullptr;
    std::shared_ptr<ov::AnyMap> options = nullptr;
};

void vlmPerformInferenceThread(VLMTsfnContext* context) {
    try {
        ov::genai::GenerationConfig config;
        config.update_generation_config(*context->generation_config);

        auto disableStreamer = false;
        if (context->options->find("disableStreamer") != context->options->end()) {
            auto value = (*context->options)["disableStreamer"];
            if (value.is<bool>()) {
                disableStreamer = value.as<bool>();
            } else {
                OPENVINO_THROW("disableStreamer option should be boolean");
            }
        }

        ov::genai::StreamerVariant streamer = std::monostate();
        if (!disableStreamer) {
            streamer = [context](std::string word) {
                std::promise<ov::genai::StreamingStatus> resultPromise;
                napi_status status = context->tsfn.BlockingCall([word, &resultPromise](Napi::Env env, Napi::Function jsCallback) {
                    try {
                        auto callback_result = jsCallback.Call({
                            Napi::Boolean::New(env, false),
                            Napi::String::New(env, word)
                        });
                        if (callback_result.IsNumber()) {
                            resultPromise.set_value(static_cast<ov::genai::StreamingStatus>(callback_result.As<Napi::Number>().Int32Value()));
                        } else {
                            resultPromise.set_value(ov::genai::StreamingStatus::RUNNING);
                        }
                    } catch(std::exception& err) {
                        Napi::Error::Fatal("vlmPerformInferenceThread callback error. Details:" , err.what());
                    }
                });
                if (status != napi_ok) {
                    Napi::Error::Fatal("vlmPerformInferenceThread error", "napi_status != napi_ok");
                }

                return resultPromise.get_future().get();
            };
        }

        ov::genai::VLMDecodedResults result;
        
        result = context->pipe->generate(context->prompt, context->images, context->videos, config, streamer);

        napi_status status = context->tsfn.BlockingCall([result](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({Napi::Boolean::New(env, true), to_vlm_decoded_result(env, result)});
        });

        if (status != napi_ok) {
            Napi::Error::Fatal("vlmPerformInferenceThread error", "napi_status != napi_ok");
        }

        context->tsfn.Release();
    }
    catch(std::exception& e) {
        Napi::Error::Fatal("vlmPerformInferenceThread error" , e.what());

        context->tsfn.Release();
    }
}

VLMPipelineWrapper::VLMPipelineWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<VLMPipelineWrapper>(info) {};

Napi::Function VLMPipelineWrapper::get_class(Napi::Env env) {
    return DefineClass(env,
                       "VLMPipeline",
                       {InstanceMethod("init", &VLMPipelineWrapper::init),
                        InstanceMethod("generate", &VLMPipelineWrapper::generate),
                        InstanceMethod("getTokenizer", &VLMPipelineWrapper::get_tokenizer),
                        InstanceMethod("startChat", &VLMPipelineWrapper::start_chat),
                        InstanceMethod("finishChat", &VLMPipelineWrapper::finish_chat),
                        InstanceMethod("setChatTemplate", &VLMPipelineWrapper::set_chat_template),
                        InstanceMethod("setGenerationConfig", &VLMPipelineWrapper::set_generation_config)});
}

Napi::Value VLMPipelineWrapper::init(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    const std::string model_path = info[0].ToString();
    const std::string device = info[1].ToString();
    const auto& properties = js_to_cpp<ov::AnyMap>(info.Env(), info[2]);
    Napi::Function callback = info[3].As<Napi::Function>();

    VLMInitWorker* asyncWorker = new VLMInitWorker(callback, this->pipe, model_path, device, properties);
    asyncWorker->Queue();

    return info.Env().Undefined();
}

Napi::Value VLMPipelineWrapper::generate(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 6, "generate()");
    Napi::Env env = info.Env();
    VLMTsfnContext* context = nullptr;

    try {
        // Arguments: prompt, images, videos, callback, generationConfig, options
        auto prompt = js_to_cpp<std::string>(env, info[0]);
        auto images = js_to_cpp<std::vector<ov::Tensor>>(env, info[1]);
        auto videos = js_to_cpp<std::vector<ov::Tensor>>(env, info[2]);
        auto async_callback = info[3].As<Napi::Function>();
        auto generation_config = js_to_cpp<ov::AnyMap>(info.Env(), info[4]);
        ov::AnyMap options = js_to_cpp<ov::AnyMap>(info.Env(), info[5]);

        context = new VLMTsfnContext(prompt);
        context->images = std::move(images);
        context->videos = std::move(videos);
        context->pipe = this->pipe;
        context->generation_config = std::make_shared<ov::AnyMap>(generation_config);
        context->options = std::make_shared<ov::AnyMap>(options);
        
        context->tsfn = Napi::ThreadSafeFunction::New(
            env,
            async_callback,                 // JavaScript function called asynchronously
            "VLM_TSFN",                     // Name
            0,                              // Unlimited queue
            1,                              // Only one thread will use this initially
            [context](Napi::Env) {          // Finalizer used to clean threads up
                context->native_thread.join();
                delete context;
            }
        );
        context->native_thread = std::thread(vlmPerformInferenceThread, context);

        return Napi::Boolean::New(env, false);
    }
    catch (const std::exception& ex) {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value VLMPipelineWrapper::start_chat(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    OPENVINO_ASSERT(
        info.Length() == 2 && info[0].IsString() && info[1].IsFunction(),
        "startChat expects 2 arguments: system_message and callback function"
    );
    auto system_message = js_to_cpp<std::string>(info.Env(), info[0]);
    auto callback = info[1].As<Napi::Function>();

    VLMStartChatWorker* asyncWorker = new VLMStartChatWorker(callback, this->pipe, system_message);
    asyncWorker->Queue();

    return info.Env().Undefined();
}

Napi::Value VLMPipelineWrapper::finish_chat(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    Napi::Function callback = info[0].As<Napi::Function>();

    VLMFinishChatWorker* asyncWorker = new VLMFinishChatWorker(callback, this->pipe);
    asyncWorker->Queue();

    return info.Env().Undefined();
}

Napi::Value VLMPipelineWrapper::get_tokenizer(const Napi::CallbackInfo& info) {
    auto tokenizer = this->pipe->get_tokenizer();

    return TokenizerWrapper::wrap(info.Env(), tokenizer);
}

Napi::Value VLMPipelineWrapper::set_chat_template(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    VALIDATE_ARGS_COUNT(info, 1, "setChatTemplate()");
    
    try {
        auto chat_template = js_to_cpp<std::string>(env, info[0]);
        this->pipe->set_chat_template(chat_template);
    } 
    catch (const std::exception& ex) {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

Napi::Value VLMPipelineWrapper::set_generation_config(const Napi::CallbackInfo& info) {
    VALIDATE_ARGS_COUNT(info, 1, "setGenerationConfig()");
    Napi::Env env = info.Env();
    
    try {
        auto config_map = js_to_cpp<ov::AnyMap>(env, info[0]);
        ov::genai::GenerationConfig config;
        config.update_generation_config(config_map);
        this->pipe->set_generation_config(config);
    } 
    catch (const std::exception& ex) {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    
    return env.Undefined();
}
