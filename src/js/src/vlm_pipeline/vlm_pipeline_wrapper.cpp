// Copyright (C) 2018-2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "include/vlm_pipeline/vlm_pipeline_wrapper.hpp"

#include <future>
#include "include/addon.hpp"
#include "include/helper.hpp"
#include "include/vlm_pipeline/vlm_perf_metrics.hpp"
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

Napi::Object create_vlm_decoded_results_object(Napi::Env env, const ov::genai::VLMDecodedResults& result) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("texts", cpp_to_js<std::vector<std::string>, Napi::Value>(env, result.texts));
    obj.Set("scores", cpp_to_js<std::vector<float>, Napi::Value>(env, result.scores));
    obj.Set("perfMetrics", VLMPerfMetricsWrapper::wrap(env, result.perf_metrics));
    obj.Set("subword", Napi::String::New(env, result));
    return obj;
}

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
            jsCallback.Call({Napi::Boolean::New(env, true), create_vlm_decoded_results_object(env, result)});
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
                        InstanceMethod("getGenerationConfig", &VLMPipelineWrapper::get_generation_config),
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

std::vector<ov::Tensor> js_array_to_tensors(const Napi::Env& env, const Napi::Value& value) {
    std::vector<ov::Tensor> tensors;
    if (value.IsUndefined() || value.IsNull()) {
        return tensors;
    }
    
    if (value.IsArray()) {
        auto array = value.As<Napi::Array>();
        size_t length = array.Length();
        tensors.reserve(length);
        for (uint32_t i = 0; i < length; ++i) {
            tensors.push_back(js_to_cpp<ov::Tensor>(env, array[i]));
        }
    } else if (value.IsObject()) {
        // Single tensor
        tensors.push_back(js_to_cpp<ov::Tensor>(env, value));
    }
    
    return tensors;
}

Napi::Value VLMPipelineWrapper::generate(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    VLMTsfnContext* context = nullptr;

    try {
        // Arguments: prompt, images, videos, callback, generationConfig, options
        auto prompt = js_to_cpp<std::string>(env, info[0]);
        auto images = js_array_to_tensors(env, info[1]);
        auto videos = js_array_to_tensors(env, info[2]);
        
        auto generation_config = js_to_cpp<ov::AnyMap>(info.Env(), info[4]);
        ov::AnyMap options;
        if (info.Length() > 5) {
            options = js_to_cpp<ov::AnyMap>(info.Env(), info[5]);
        }

        context = new VLMTsfnContext(prompt);
        context->images = std::move(images);
        context->videos = std::move(videos);
        context->pipe = this->pipe;
        context->generation_config = std::make_shared<ov::AnyMap>(generation_config);
        context->options = std::make_shared<ov::AnyMap>(options);
        
        // Create a ThreadSafeFunction
        context->tsfn = Napi::ThreadSafeFunction::New(
            env,
            info[3].As<Napi::Function>(),   // JavaScript function called asynchronously
            "VLMTSFN",                      // Name
            0,                              // Unlimited queue
            1,                              // Only one thread will use this initially
            [context](Napi::Env) {          // Finalizer used to clean threads up
                context->native_thread.join();
                delete context;
            }
        );
        context->native_thread = std::thread(vlmPerformInferenceThread, context);

        return Napi::Boolean::New(env, false);
    } catch(Napi::TypeError& type_err) {
        throw type_err;
    } catch(std::exception& err) {
        std::cout << "Catch in the thread: '" << err.what() << "'" << std::endl;
        if (context != nullptr) {
            context->tsfn.Release();
        }

        throw Napi::Error::New(env, err.what());
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
    OPENVINO_ASSERT(info.Length() == 1 && info[0].IsString(), "setChatTemplate expects 1 string argument");
    
    auto chat_template = js_to_cpp<std::string>(env, info[0]);
    this->pipe->set_chat_template(chat_template);
    
    return env.Undefined();
}

Napi::Value VLMPipelineWrapper::get_generation_config(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto config = this->pipe->get_generation_config();
    
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("max_new_tokens", Napi::Number::New(env, config.max_new_tokens));
    obj.Set("max_length", Napi::Number::New(env, config.max_length));
    obj.Set("temperature", Napi::Number::New(env, config.temperature));
    obj.Set("top_p", Napi::Number::New(env, config.top_p));
    obj.Set("top_k", Napi::Number::New(env, config.top_k));
    obj.Set("do_sample", Napi::Boolean::New(env, config.do_sample));
    obj.Set("repetition_penalty", Napi::Number::New(env, config.repetition_penalty));
    obj.Set("num_return_sequences", Napi::Number::New(env, config.num_return_sequences));
    
    return obj;
}

Napi::Value VLMPipelineWrapper::set_generation_config(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    OPENVINO_ASSERT(info.Length() == 1 && info[0].IsObject(), "setGenerationConfig expects 1 object argument");
    
    auto config_map = js_to_cpp<ov::AnyMap>(env, info[0]);
    ov::genai::GenerationConfig config;
    config.update_generation_config(config_map);
    this->pipe->set_generation_config(config);
    
    return env.Undefined();
}
