#pragma once
#include <napi.h>
#include <memory>
#include <optional>
#include "openvino/genai/generation_config.hpp"

class StructuredOutputConfigWrap : public Napi::ObjectWrap<StructuredOutputConfigWrap> {
public:
    class RegexWrap : public Napi::ObjectWrap<RegexWrap> {
    public:
        static Napi::FunctionReference ctor;
        static Napi::Function get_class(Napi::Env env);
        static Napi::Object wrap(Napi::Env env, const ov::genai::StructuredOutputConfig::Regex value);

        RegexWrap(const Napi::CallbackInfo& info);
        Napi::Value to_string(const Napi::CallbackInfo& info) {
            return Napi::String::New(info.Env(), value.to_string());
        }
        ov::genai::StructuredOutputConfig::Regex get_value() {
            return value;
        }

    private:
        ov::genai::StructuredOutputConfig::Regex value;
    };

    class JSONSchemaWrap : public Napi::ObjectWrap<JSONSchemaWrap> {
    public:
        static Napi::FunctionReference ctor;
        static Napi::Function get_class(Napi::Env env);
        static Napi::Object wrap(Napi::Env env, const ov::genai::StructuredOutputConfig::JSONSchema value);

        JSONSchemaWrap(const Napi::CallbackInfo& info);
        Napi::Value to_string(const Napi::CallbackInfo& info) { 
            return Napi::String::New(info.Env(), value.to_string());
        }
        ov::genai::StructuredOutputConfig::JSONSchema get_value() {
            return value;
        }

    private:
        ov::genai::StructuredOutputConfig::JSONSchema value;
    };

    class EBNFWrap : public Napi::ObjectWrap<EBNFWrap> {
    public:
        static Napi::FunctionReference ctor;
        static Napi::Function get_class(Napi::Env env);
        static Napi::Object wrap(Napi::Env env, const ov::genai::StructuredOutputConfig::EBNF value);

        EBNFWrap(const Napi::CallbackInfo& info);
        Napi::Value to_string(const Napi::CallbackInfo& info) {
            return Napi::String::New(info.Env(), value.to_string());
        }
        ov::genai::StructuredOutputConfig::EBNF get_value() {
            return value;
        }

    private:
        ov::genai::StructuredOutputConfig::EBNF value;

    };

    class ConstStringWrap : public Napi::ObjectWrap<ConstStringWrap> {
    public:
        static Napi::FunctionReference ctor;
        static Napi::Function get_class(Napi::Env env);
        static Napi::Object wrap(Napi::Env env, const ov::genai::StructuredOutputConfig::ConstString value);

        ConstStringWrap(const Napi::CallbackInfo& info);
        Napi::Value to_string(const Napi::CallbackInfo& info) {
            return Napi::String::New(info.Env(), value.to_string());
        }
        ov::genai::StructuredOutputConfig::ConstString get_value() {
            return value;
        }

    private:
        ov::genai::StructuredOutputConfig::ConstString value;
    };

    class AnyTextWrap : public Napi::ObjectWrap<AnyTextWrap> {
    public:
        static Napi::FunctionReference ctor;
        static Napi::Function get_class(Napi::Env env);
        static Napi::Object wrap(Napi::Env env, const ov::genai::StructuredOutputConfig::AnyText value);

        AnyTextWrap(const Napi::CallbackInfo& info);
        Napi::Value to_string(const Napi::CallbackInfo& info) {
            return Napi::String::New(info.Env(), value.to_string());
        }
        ov::genai::StructuredOutputConfig::AnyText get_value() {
            return value;
        }

    private:
        ov::genai::StructuredOutputConfig::AnyText value;
    };

    class QwenXMLParametersFormatWrap : public Napi::ObjectWrap<QwenXMLParametersFormatWrap> {
    public:
        static Napi::FunctionReference ctor;
        static Napi::Function get_class(Napi::Env env);
        static Napi::Object wrap(Napi::Env env, const ov::genai::StructuredOutputConfig::QwenXMLParametersFormat value);

        QwenXMLParametersFormatWrap(const Napi::CallbackInfo& info);
        Napi::Value to_json(const Napi::CallbackInfo& info) {
            return Napi::String::New(info.Env(), value.to_json());
        }
        Napi::Value to_string(const Napi::CallbackInfo& info) {
            return Napi::String::New(info.Env(), value.to_string());
        }
        ov::genai::StructuredOutputConfig::QwenXMLParametersFormat get_value() {
            return value;
        }

    private:
        ov::genai::StructuredOutputConfig::QwenXMLParametersFormat value;
    };

    class ConcatWrap : public Napi::ObjectWrap<ConcatWrap> {
    public:
        static Napi::FunctionReference ctor;
        static Napi::Function get_class(Napi::Env env);
        static Napi::Object wrap(Napi::Env env, const std::shared_ptr<ov::genai::StructuredOutputConfig::Concat> value);

        ConcatWrap(const Napi::CallbackInfo& info);
        Napi::Value to_json(const Napi::CallbackInfo& info) {
            return Napi::String::New(info.Env(), ptr->to_json());
        }
        Napi::Value to_string(const Napi::CallbackInfo& info) {
            return Napi::String::New(info.Env(), ptr->to_string());
        }
        std::shared_ptr<ov::genai::StructuredOutputConfig::Concat> get_value() {
            return ptr;
        }

    private:
        std::shared_ptr<ov::genai::StructuredOutputConfig::Concat> ptr;
    };

    class UnionWrap : public Napi::ObjectWrap<UnionWrap> {
    public:
        static Napi::FunctionReference ctor;
        static Napi::Function get_class(Napi::Env env);
        static Napi::Object wrap(Napi::Env env, const std::shared_ptr<ov::genai::StructuredOutputConfig::Union> value);

        UnionWrap(const Napi::CallbackInfo& info);
        Napi::Value to_json(const Napi::CallbackInfo& info) {
            return Napi::String::New(info.Env(), ptr->to_json());
        }
        Napi::Value to_string(const Napi::CallbackInfo& info) {
            return Napi::String::New(info.Env(), ptr->to_string());
        }
        std::shared_ptr<ov::genai::StructuredOutputConfig::Union> get_value() {
            return ptr;
        }

    private:
        std::shared_ptr<ov::genai::StructuredOutputConfig::Union> ptr;
    };

    class TagWrap : public Napi::ObjectWrap<TagWrap> {
    public:
        static Napi::FunctionReference ctor;
        static Napi::Function get_class(Napi::Env env);
        static Napi::Object wrap(Napi::Env env, const std::shared_ptr<ov::genai::StructuredOutputConfig::Tag> value);

        TagWrap(const Napi::CallbackInfo& info);
        Napi::Value to_json(const Napi::CallbackInfo& info) {
            return Napi::String::New(info.Env(), ptr->to_json());
        }
        Napi::Value to_string(const Napi::CallbackInfo& info) {
            return Napi::String::New(info.Env(), ptr->to_string());
        }
        std::shared_ptr<ov::genai::StructuredOutputConfig::Tag> get_value() {
            return ptr;
        }

    private:
        std::shared_ptr<ov::genai::StructuredOutputConfig::Tag> ptr;
    };

    class TriggeredTagsWrap : public Napi::ObjectWrap<TriggeredTagsWrap> {
    public:
        static Napi::FunctionReference ctor;
        static Napi::Function get_class(Napi::Env env);
        static Napi::Object wrap(Napi::Env env, const std::shared_ptr<ov::genai::StructuredOutputConfig::TriggeredTags> value);

        TriggeredTagsWrap(const Napi::CallbackInfo& info);
        Napi::Value to_json(const Napi::CallbackInfo& info) {
            return Napi::String::New(info.Env(), ptr->to_json());
        }
        Napi::Value to_string(const Napi::CallbackInfo& info) {
            return Napi::String::New(info.Env(), ptr->to_string());
        }
        std::shared_ptr<ov::genai::StructuredOutputConfig::TriggeredTags> get_value() {
            return ptr;
        }

    private:
        std::shared_ptr<ov::genai::StructuredOutputConfig::TriggeredTags> ptr;
    };

    class TagsWithSeparatorWrap : public Napi::ObjectWrap<TagsWithSeparatorWrap> {
    public:
        static Napi::FunctionReference ctor;
        static Napi::Function get_class(Napi::Env env);
        static Napi::Object wrap(Napi::Env env, const std::shared_ptr<ov::genai::StructuredOutputConfig::TagsWithSeparator> value);

        TagsWithSeparatorWrap(const Napi::CallbackInfo& info);
        Napi::Value to_json(const Napi::CallbackInfo& info) {
            return Napi::String::New(info.Env(), ptr->to_json());
        }
        Napi::Value to_string(const Napi::CallbackInfo& info) {
            return Napi::String::New(info.Env(), ptr->to_string());
        }
        std::shared_ptr<ov::genai::StructuredOutputConfig::TagsWithSeparator> get_value() {
            return ptr;
        }

    private:
        std::shared_ptr<ov::genai::StructuredOutputConfig::TagsWithSeparator> ptr;
    };

    static Napi::Function get_class(Napi::Env env);

    StructuredOutputConfigWrap(const Napi::CallbackInfo& info);
    ~StructuredOutputConfigWrap() {
        std::cout << "StructuredOutputConfigWrap Destructor Called" << std::endl;
    }

    Napi::Value get_json_schema(const Napi::CallbackInfo& info);
    void set_json_schema(const Napi::CallbackInfo& info, const Napi::Value& value);
    Napi::Value get_regex(const Napi::CallbackInfo& info);
    void set_regex(const Napi::CallbackInfo& info, const Napi::Value& value);
    Napi::Value get_grammar(const Napi::CallbackInfo& info);
    void set_grammar(const Napi::CallbackInfo& info, const Napi::Value& value);
    Napi::Value get_structural_tags_config(const Napi::CallbackInfo& info);
    void set_structural_tags_config(const Napi::CallbackInfo& info, const Napi::Value& value);
    std::shared_ptr<ov::genai::StructuredOutputConfig> get_value() {
        return ptr;
    }
    Napi::Value to_string(const Napi::CallbackInfo& info) { return Napi::String::New(info.Env(), "<<StructuredOutputConfig>>"); }

private:
    std::shared_ptr<ov::genai::StructuredOutputConfig> ptr;
};
