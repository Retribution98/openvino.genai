#pragma once
#include <napi.h>

#include "structured_output_config.hpp"

template <typename T>
bool validate_value(const Napi::Env& env, const Napi::Value& arg) {
    OPENVINO_THROW("Validation for this type is not implemented!");
};

template <>
bool validate_value<StructuredOutputConfigWrap>(const Napi::Env& env, const Napi::Value& value);
