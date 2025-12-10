// Copyright (C) 2018-2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "include/vlm_pipeline/vlm_pipeline_wrapper.hpp"

#include <future>

#include "include/addon.hpp"
#include "include/helper.hpp"
#include "include/tokenizer.hpp"
#include "include/vlm_pipeline/finish_chat_worker.hpp"
#include "include/vlm_pipeline/init_worker.hpp"
#include "include/vlm_pipeline/perf_metrics.hpp"
#include "include/vlm_pipeline/start_chat_worker.hpp"

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
    auto report_error = [context](const std::string& message) {
        auto status = context->tsfn.BlockingCall([message](Napi::Env env, Napi::Function jsCallback) {
            try {
                jsCallback.Call({Napi::Error::New(env, "vlmPerformInferenceThread error. " + message).Value(),
                                 env.Undefined(),
                                 env.Undefined()});
            } catch (std::exception& err) {
                std::cerr << "The callback failed when attempting to return an error from vlmPerformInferenceThread. "
                             "Details:\n"
                          << err.what() << std::endl;
                std::cerr << "Original error message:\n" << message << std::endl;
            }
        });
        if (status != napi_ok) {
            std::cerr << "The BlockingCall failed with status " << status
                      << " when trying to return an error from vlmPerformInferenceThread." << std::endl;
            std::cerr << "Original error message:\n" << message << std::endl;
        }
    };
    try {
        ov::genai::GenerationConfig config;
        config.update_generation_config(*context->generation_config);

        auto disableStreamer = false;
        if (context->options->find("disableStreamer") != context->options->end()) {
            auto value = (*context->options)["disableStreamer"];
            OPENVINO_ASSERT(value.is<bool>(), "disableStreamer option should be boolean");
            disableStreamer = value.as<bool>();
        }

        ov::genai::StreamerVariant streamer = std::monostate();
        std::vector<std::string> streamer_exceptions;
        if (!disableStreamer) {
            streamer = [context, &streamer_exceptions](std::string word) {
                std::promise<ov::genai::StreamingStatus> resultPromise;
                napi_status status = context->tsfn.BlockingCall(
                    [word, &resultPromise, &streamer_exceptions](Napi::Env env, Napi::Function jsCallback) {
                        try {
                            auto callback_result =
                                jsCallback.Call({env.Undefined(),  // Error should be undefined in normal case
                                                 Napi::Boolean::New(env, false),
                                                 Napi::String::New(env, word)});
                            if (callback_result.IsNumber()) {
                                resultPromise.set_value(static_cast<ov::genai::StreamingStatus>(
                                    callback_result.As<Napi::Number>().Int32Value()));
                            } else {
                                resultPromise.set_value(ov::genai::StreamingStatus::RUNNING);
                            }
                        } catch (std::exception& err) {
                            streamer_exceptions.push_back(err.what());
                            resultPromise.set_value(ov::genai::StreamingStatus::CANCEL);
                        }
                    });

                if (status != napi_ok) {
                    streamer_exceptions.push_back("The streamer callback BlockingCall failed with the status: " +
                                                  status);
                    return ov::genai::StreamingStatus::CANCEL;
                }

                return resultPromise.get_future().get();
            };
        }

        ov::genai::VLMDecodedResults result;

        result = context->pipe->generate(context->prompt, context->images, context->videos, config, streamer);

        if (!streamer_exceptions.empty()) {
            // If there were exceptions from the streamer, report them all as a single error and finish without result
            std::string combined_error = "Streamer exceptions occurred:\n";
            for (size_t i = 0; i < streamer_exceptions.size(); ++i) {
                combined_error += "[" + std::to_string(i + 1) + "] " + streamer_exceptions[i] + "\n";
            }
            report_error(combined_error);
        } else {
            // If no exceptions from streamer, call the final callback with the result
            napi_status status =
                context->tsfn.BlockingCall([result, &report_error](Napi::Env env, Napi::Function jsCallback) {
                    try {
                        jsCallback.Call({env.Undefined(),  // Error should be undefined in normal case
                                         Napi::Boolean::New(env, true),
                                         to_vlm_decoded_result(env, result)});
                    } catch (std::exception& err) {
                        report_error("The final callback failed. Details:\n" + std::string(err.what()));
                    }
                });

            if (status != napi_ok) {
                report_error("The final BlockingCall failed with status " + status);
            }
        }
        context->tsfn.Release();
    } catch (std::exception& e) {
        report_error(e.what());
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
    auto env = info.Env();
    try {
        OPENVINO_ASSERT(!this->pipe, "Pipeline is already initialized");
        OPENVINO_ASSERT(!*this->is_initializing, "Pipeline is already initializing");
        VALIDATE_ARGS_COUNT(info, 4, "init()");
        const std::string model_path = js_to_cpp<std::string>(env, info[0]);
        const std::string device = js_to_cpp<std::string>(env, info[1]);
        const auto& properties = js_to_cpp<ov::AnyMap>(env, info[2]);
        OPENVINO_ASSERT(info[3].IsFunction(), "init callback is not a function");
        Napi::Function callback = info[3].As<Napi::Function>();

        VLMInitWorker* asyncWorker =
            new VLMInitWorker(callback, this->pipe, this->is_initializing, model_path, device, properties);
        asyncWorker->Queue();
    } catch (const std::exception& ex) {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

Napi::Value VLMPipelineWrapper::generate(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    try {
        VALIDATE_ARGS_COUNT(info, 6, "generate()");
        VLMTsfnContext* context = nullptr;

        // Arguments: prompt, images, videos, callback, generationConfig, options
        auto prompt = js_to_cpp<std::string>(env, info[0]);
        auto images = js_to_cpp<std::vector<ov::Tensor>>(env, info[1]);
        auto videos = js_to_cpp<std::vector<ov::Tensor>>(env, info[2]);
        auto async_callback = info[3].As<Napi::Function>();
        auto generation_config = js_to_cpp<ov::AnyMap>(env, info[4]);
        ov::AnyMap options = js_to_cpp<ov::AnyMap>(env, info[5]);

        context = new VLMTsfnContext(prompt);
        context->images = std::move(images);
        context->videos = std::move(videos);
        context->pipe = this->pipe;
        context->generation_config = std::make_shared<ov::AnyMap>(generation_config);
        context->options = std::make_shared<ov::AnyMap>(options);

        context->tsfn = Napi::ThreadSafeFunction::New(env,
                                                      async_callback,  // JavaScript function called asynchronously
                                                      "VLM_TSFN",      // Name
                                                      0,               // Unlimited queue
                                                      1,               // Only one thread will use this initially
                                                      [context](Napi::Env) {  // Finalizer used to clean threads up
                                                          context->native_thread.join();
                                                          delete context;
                                                      });
        context->native_thread = std::thread(vlmPerformInferenceThread, context);
    } catch (const std::exception& ex) {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

Napi::Value VLMPipelineWrapper::start_chat(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    try {
        VALIDATE_ARGS_COUNT(info, 2, "startChat()");
        auto system_message = js_to_cpp<std::string>(env, info[0]);
        OPENVINO_ASSERT(info[1].IsFunction(), "startChat callback is not a function");
        auto callback = info[1].As<Napi::Function>();

        VLMStartChatWorker* asyncWorker = new VLMStartChatWorker(callback, this->pipe, system_message);
        asyncWorker->Queue();
    } catch (const std::exception& ex) {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

Napi::Value VLMPipelineWrapper::finish_chat(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    try {
        VALIDATE_ARGS_COUNT(info, 1, "finishChat()");
        OPENVINO_ASSERT(info[0].IsFunction(), "finishChat callback is not a function");
        Napi::Function callback = info[0].As<Napi::Function>();

        VLMFinishChatWorker* asyncWorker = new VLMFinishChatWorker(callback, this->pipe);
        asyncWorker->Queue();
    } catch (const std::exception& ex) {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

Napi::Value VLMPipelineWrapper::get_tokenizer(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    try {
        auto tokenizer = this->pipe->get_tokenizer();
        return TokenizerWrapper::wrap(env, tokenizer);
    } catch (const std::exception& ex) {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

Napi::Value VLMPipelineWrapper::set_chat_template(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    try {
        VALIDATE_ARGS_COUNT(info, 1, "setChatTemplate()");
        auto chat_template = js_to_cpp<std::string>(env, info[0]);
        this->pipe->set_chat_template(chat_template);
    } catch (const std::exception& ex) {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

Napi::Value VLMPipelineWrapper::set_generation_config(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    try {
        VALIDATE_ARGS_COUNT(info, 1, "setGenerationConfig()");
        auto config_map = js_to_cpp<ov::AnyMap>(env, info[0]);
        ov::genai::GenerationConfig config;
        config.update_generation_config(config_map);
        this->pipe->set_generation_config(config);
    } catch (const std::exception& ex) {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}
