#include "include/addon.hpp"
#include "include/type_validation.hpp"

template <>
bool validate_value<StructuredOutputConfigWrap>(const Napi::Env& env, const Napi::Value& value) {
    const auto& prototype = env.GetInstanceData<AddonData>()->structured_output_config;
    return value.ToObject().InstanceOf(prototype.Value().As<Napi::Function>());
}

template <>
bool validate_value<StructuredOutputConfigWrap::ConcatWrap>(const Napi::Env& env, const Napi::Value& value) {
    const auto& prototype = StructuredOutputConfigWrap::ConcatWrap::ctor;
    return value.ToObject().InstanceOf(prototype.Value().As<Napi::Function>());
}

template <>
bool validate_value<StructuredOutputConfigWrap::AnyTextWrap>(const Napi::Env& env, const Napi::Value& value) {
    const auto& prototype = StructuredOutputConfigWrap::AnyTextWrap::ctor;
    return value.ToObject().InstanceOf(prototype.Value().As<Napi::Function>());
}

template <>
bool validate_value<StructuredOutputConfigWrap::ConstStringWrap>(const Napi::Env& env, const Napi::Value& value) {
    const auto& prototype = StructuredOutputConfigWrap::ConstStringWrap::ctor;
    return value.ToObject().InstanceOf(prototype.Value().As<Napi::Function>());
}

template <>
bool validate_value<StructuredOutputConfigWrap::EBNFWrap>(const Napi::Env& env, const Napi::Value& value) {
    const auto& prototype = StructuredOutputConfigWrap::EBNFWrap::ctor;
    return value.ToObject().InstanceOf(prototype.Value().As<Napi::Function>());
}

template <>
bool validate_value<StructuredOutputConfigWrap::JSONSchemaWrap>(const Napi::Env& env, const Napi::Value& value) {
    const auto& prototype = StructuredOutputConfigWrap::JSONSchemaWrap::ctor;
    return value.ToObject().InstanceOf(prototype.Value().As<Napi::Function>());
}

template <>
bool validate_value<StructuredOutputConfigWrap::QwenXMLParametersFormatWrap>(const Napi::Env& env, const Napi::Value& value) {
    const auto& prototype = StructuredOutputConfigWrap::QwenXMLParametersFormatWrap::ctor;
    return value.ToObject().InstanceOf(prototype.Value().As<Napi::Function>());
}

template <>
bool validate_value<StructuredOutputConfigWrap::RegexWrap>(const Napi::Env& env, const Napi::Value& value) {
    const auto& prototype = StructuredOutputConfigWrap::RegexWrap::ctor;
    return value.ToObject().InstanceOf(prototype.Value().As<Napi::Function>());
}

template <>
bool validate_value<StructuredOutputConfigWrap::TagsWithSeparatorWrap>(const Napi::Env& env, const Napi::Value& value) {
    const auto& prototype = StructuredOutputConfigWrap::TagsWithSeparatorWrap::ctor;
    return value.ToObject().InstanceOf(prototype.Value().As<Napi::Function>());
}

template <>
bool validate_value<StructuredOutputConfigWrap::TagWrap>(const Napi::Env& env, const Napi::Value& value) {
    const auto& prototype = StructuredOutputConfigWrap::TagWrap::ctor;
    return value.ToObject().InstanceOf(prototype.Value().As<Napi::Function>());
}

template <>
bool validate_value<StructuredOutputConfigWrap::TriggeredTagsWrap>(const Napi::Env& env, const Napi::Value& value) {
    const auto& prototype = StructuredOutputConfigWrap::TriggeredTagsWrap::ctor;
    return value.ToObject().InstanceOf(prototype.Value().As<Napi::Function>());
}

template <>
bool validate_value<StructuredOutputConfigWrap::UnionWrap>(const Napi::Env& env, const Napi::Value& value) {
    const auto& prototype = StructuredOutputConfigWrap::UnionWrap::ctor;
    return value.ToObject().InstanceOf(prototype.Value().As<Napi::Function>());
}
