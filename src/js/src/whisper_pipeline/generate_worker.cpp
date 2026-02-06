// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "include/whisper_pipeline/generate_worker.hpp"

#include <future>

#include "include/helper.hpp"

WhisperGenerateWorker::WhisperGenerateWorker(Napi::Function& callback,
                                             std::shared_ptr<ov::genai::WhisperPipeline> pipe,
                                             std::shared_ptr<bool> is_generating,
                                             std::vector<float>&& raw_speech,
                                             ov::AnyMap&& generation_config,
                                             std::optional<Napi::ThreadSafeFunction> streamer_tsfn)
    : Napi::AsyncWorker(callback),
      pipe(std::move(pipe)),
      is_generating(is_generating),
      raw_speech(std::move(raw_speech)),
      generation_config(std::move(generation_config)),
      streamer_tsfn(std::move(streamer_tsfn)) {}

void WhisperGenerateWorker::Execute() {
    ov::genai::StreamerVariant streamer_var = std::monostate();
    std::vector<std::string> streamer_exceptions;

    if (streamer_tsfn.has_value()) {
        streamer_var = [this, &streamer_exceptions](std::string word) {
            std::promise<ov::genai::StreamingStatus> result_promise;
            napi_status status = streamer_tsfn->BlockingCall(
                [word, &result_promise, &streamer_exceptions](Napi::Env env, Napi::Function js_callback) {
                    try {
                        auto callback_result = js_callback.Call({Napi::String::New(env, word)});
                        if (callback_result.IsNumber()) {
                            result_promise.set_value(static_cast<ov::genai::StreamingStatus>(
                                callback_result.As<Napi::Number>().Int32Value()));
                        } else {
                            result_promise.set_value(ov::genai::StreamingStatus::RUNNING);
                        }
                    } catch (const std::exception& err) {
                        streamer_exceptions.push_back(err.what());
                        result_promise.set_value(ov::genai::StreamingStatus::CANCEL);
                    }
                });

            if (status != napi_ok) {
                streamer_exceptions.push_back("The streamer callback BlockingCall failed with status: " +
                                               std::to_string(status));
                return ov::genai::StreamingStatus::CANCEL;
            }
            return result_promise.get_future().get();
        };
    }

    if (!streamer_exceptions.empty()) {
        SetError(streamer_exceptions.front());
        return;
    }

    if (streamer_tsfn.has_value()) {
        auto config = this->pipe->get_generation_config();
        config.update_generation_config(this->generation_config);
        this->result = this->pipe->generate(this->raw_speech, config, streamer_var);
    } else {
        this->result = this->pipe->generate(this->raw_speech, this->generation_config);
    }
}

void WhisperGenerateWorker::OnOK() {
    *this->is_generating = false;
    if (streamer_tsfn.has_value()) {
        streamer_tsfn->Release();
    }
    Callback().Call({Env().Null(), to_whisper_decoded_result(Env(), this->result)});
}

void WhisperGenerateWorker::OnError(const Napi::Error& e) {
    *this->is_generating = false;
    if (streamer_tsfn.has_value()) {
        streamer_tsfn->Release();
    }
    Callback().Call({Napi::Error::New(Env(), e.Message()).Value()});
}
