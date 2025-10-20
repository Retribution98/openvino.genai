#include "include/helper.hpp"
#include "include/structured_output_config.hpp"
#include <sstream>

Napi::FunctionReference StructuredOutputConfigWrap::RegexWrap::ctor;
Napi::FunctionReference StructuredOutputConfigWrap::JSONSchemaWrap::ctor;
Napi::FunctionReference StructuredOutputConfigWrap::EBNFWrap::ctor;
Napi::FunctionReference StructuredOutputConfigWrap::ConstStringWrap::ctor;
Napi::FunctionReference StructuredOutputConfigWrap::AnyTextWrap::ctor;
Napi::FunctionReference StructuredOutputConfigWrap::QwenXMLParametersFormatWrap::ctor;
Napi::FunctionReference StructuredOutputConfigWrap::ConcatWrap::ctor;
Napi::FunctionReference StructuredOutputConfigWrap::UnionWrap::ctor;
Napi::FunctionReference StructuredOutputConfigWrap::TagWrap::ctor;
Napi::FunctionReference StructuredOutputConfigWrap::TriggeredTagsWrap::ctor;
Napi::FunctionReference StructuredOutputConfigWrap::TagsWithSeparatorWrap::ctor;

Napi::Function StructuredOutputConfigWrap::get_class(Napi::Env env) {
    Napi::Function func = DefineClass(env, "StructuredOutputConfig", {
        InstanceAccessor<&StructuredOutputConfigWrap::get_json_schema, &StructuredOutputConfigWrap::set_json_schema>("json_schema"),
        InstanceAccessor<&StructuredOutputConfigWrap::get_regex, &StructuredOutputConfigWrap::set_regex>("regex"),
        InstanceAccessor<&StructuredOutputConfigWrap::get_grammar, &StructuredOutputConfigWrap::set_grammar>("grammar"),
        InstanceAccessor<&StructuredOutputConfigWrap::get_structural_tags_config, &StructuredOutputConfigWrap::set_structural_tags_config>("structural_tags_config"),
        InstanceMethod("toString", &StructuredOutputConfigWrap::to_string),
    });

    func.Set("Regex", RegexWrap::get_class(env));
    func.Set("JSONSchema", JSONSchemaWrap::get_class(env));
    func.Set("EBNF", EBNFWrap::get_class(env));
    func.Set("ConstString", ConstStringWrap::get_class(env));
    func.Set("AnyText", AnyTextWrap::get_class(env));
    func.Set("QwenXMLParametersFormat", QwenXMLParametersFormatWrap::get_class(env));
    func.Set("Concat", ConcatWrap::get_class(env));
    func.Set("Union", UnionWrap::get_class(env));
    func.Set("Tag", TagWrap::get_class(env));
    func.Set("TriggeredTags", TriggeredTagsWrap::get_class(env));
    func.Set("TagsWithSeparator", TagsWithSeparatorWrap::get_class(env));

    return func;
}
StructuredOutputConfigWrap::StructuredOutputConfigWrap(const Napi::CallbackInfo& info)
: Napi::ObjectWrap<StructuredOutputConfigWrap>(info) {
    OPENVINO_ASSERT(info.Length() == 1 && info[0].IsObject(), "StructuredOutputConfig constructor accept an object.");
    auto params = js_to_cpp<ov::AnyMap>(info.Env(), info[0]);
    ptr = std::make_shared<ov::genai::StructuredOutputConfig>(params);
}
Napi::Value StructuredOutputConfigWrap::get_json_schema(const Napi::CallbackInfo& info) {
    if (ptr->json_schema.has_value()) {
        return Napi::String::New(info.Env(), ptr->json_schema.value());
    }
    return info.Env().Undefined();
}
void StructuredOutputConfigWrap::set_json_schema(const Napi::CallbackInfo& info, const Napi::Value& value) {
    OPENVINO_ASSERT(info.Length() == 1 && info[0].IsString(), "json_schema must be a string.");
    ptr->json_schema = js_to_cpp<std::string>(info.Env(), info[0]);
}
Napi::Value StructuredOutputConfigWrap::get_regex(const Napi::CallbackInfo& info) {
    if (ptr->regex.has_value()) {
        return Napi::String::New(info.Env(), ptr->regex.value());
    }
    return info.Env().Undefined();
}
void StructuredOutputConfigWrap::set_regex(const Napi::CallbackInfo& info, const Napi::Value& value) {
    OPENVINO_ASSERT(info.Length() == 1 && info[0].IsString(), "regex must be a string.");
    ptr->regex = js_to_cpp<std::string>(info.Env(), info[0]);
}
Napi::Value StructuredOutputConfigWrap::get_grammar(const Napi::CallbackInfo& info) {
    if (ptr->grammar.has_value()) {
        return Napi::String::New(info.Env(), ptr->grammar.value());
    }
    return info.Env().Undefined();
}
void StructuredOutputConfigWrap::set_grammar(const Napi::CallbackInfo& info, const Napi::Value& value) {
    OPENVINO_ASSERT(info.Length() == 1 && info[0].IsString(), "grammar must be a string.");
    ptr->grammar = js_to_cpp<std::string>(info.Env(), info[0]);
}
Napi::Value StructuredOutputConfigWrap::get_structural_tags_config(const Napi::CallbackInfo& info) {
    if (ptr->structural_tags_config.has_value()) {
        auto structural_tags_config = ptr->structural_tags_config.value();
        auto env = info.Env();
        return std::visit(overloaded{[env](const ov::genai::StructuralTagsConfig& config) {
            // JS API doesn't support StructuralTagsConfig variant.
            // This option is deprecated and will be removed in future releases.
            Napi::Error::New(env, "JS API doesn't support StructuralTagsConfig variant. "
                "This option is deprecated and will be removed in future releases. "
                "Please use StructuredOutputConfig.StructuralTag variant instead."
            ).ThrowAsJavaScriptException();
            return env.Null();
        }, [env](const ov::genai::StructuredOutputConfig::StructuralTag& tag) {
            return cpp_to_js<ov::genai::StructuredOutputConfig::StructuralTag, Napi::Value>(env, tag);
        }}, structural_tags_config);
    }
    return info.Env().Undefined();
}
void StructuredOutputConfigWrap::set_structural_tags_config(const Napi::CallbackInfo& info, const Napi::Value& value) {
    OPENVINO_ASSERT(info.Length() == 1 && info[0].IsString(), "structural_tags_config must be a string.");
    ptr->structural_tags_config = js_to_cpp<std::string>(info.Env(), info[0]);
}

Napi::Function StructuredOutputConfigWrap::RegexWrap::get_class(Napi::Env env) {
    Napi::Function f = DefineClass(env, "Regex", {
        InstanceMethod("toString", &RegexWrap::to_string)
    });
    ctor = Napi::Persistent(f);
    return f;
}
StructuredOutputConfigWrap::RegexWrap::RegexWrap(const Napi::CallbackInfo& info)
: Napi::ObjectWrap<RegexWrap>(info) {
    OPENVINO_ASSERT(info.Length() == 1, "Regex constructor requires a single string argument.");
    if (info[0].IsString()) {
        value = ov::genai::StructuredOutputConfig::Regex(js_to_cpp<std::string>(info.Env(), info[0]));
    }
}
Napi::Object StructuredOutputConfigWrap::RegexWrap::wrap(Napi::Env env, const ov::genai::StructuredOutputConfig::Regex value) {
    auto ctor = StructuredOutputConfigWrap::RegexWrap::ctor.Value();
    auto instance = ctor.New({}); 
    auto wrap = Napi::ObjectWrap<StructuredOutputConfigWrap::RegexWrap>::Unwrap(instance);
    wrap->value = value;
    return instance;
}

Napi::Function StructuredOutputConfigWrap::JSONSchemaWrap::get_class(Napi::Env env) {
    Napi::Function f = DefineClass(env, "JSONSchema", { 
        InstanceMethod("toString", &JSONSchemaWrap::to_string)
    });
    ctor = Napi::Persistent(f);
    return f;
}
StructuredOutputConfigWrap::JSONSchemaWrap::JSONSchemaWrap(const Napi::CallbackInfo& info)
: Napi::ObjectWrap<JSONSchemaWrap>(info) {
    OPENVINO_ASSERT(info.Length() == 1, "JSONSchema constructor requires a single string argument.");
    if (info[0].IsString()) {
        value = ov::genai::StructuredOutputConfig::JSONSchema(js_to_cpp<std::string>(info.Env(), info[0]));
    }
}
Napi::Object StructuredOutputConfigWrap::JSONSchemaWrap::wrap(Napi::Env env, const ov::genai::StructuredOutputConfig::JSONSchema value) {
    auto ctor = StructuredOutputConfigWrap::JSONSchemaWrap::ctor.Value();
    auto instance = ctor.New({}); 
    auto wrap = Napi::ObjectWrap<StructuredOutputConfigWrap::JSONSchemaWrap>::Unwrap(instance);
    wrap->value = value;
    return instance;
}

Napi::Function StructuredOutputConfigWrap::EBNFWrap::get_class(Napi::Env env) {
    Napi::Function f = DefineClass(env, "EBNF", { 
        InstanceMethod("toString", &EBNFWrap::to_string)
    });
    ctor = Napi::Persistent(f);
    return f;
}
StructuredOutputConfigWrap::EBNFWrap::EBNFWrap(const Napi::CallbackInfo& info)
: Napi::ObjectWrap<EBNFWrap>(info) {
    OPENVINO_ASSERT(info.Length() == 1, "EBNF constructor requires a single string argument.");
    if (info[0].IsString()) {
        value = ov::genai::StructuredOutputConfig::EBNF(js_to_cpp<std::string>(info.Env(), info[0]));
    }
}
Napi::Object StructuredOutputConfigWrap::EBNFWrap::wrap(Napi::Env env, const ov::genai::StructuredOutputConfig::EBNF value) {
    auto ctor = StructuredOutputConfigWrap::EBNFWrap::ctor.Value();
    auto instance = ctor.New({}); 
    auto wrap = Napi::ObjectWrap<StructuredOutputConfigWrap::EBNFWrap>::Unwrap(instance);
    wrap->value = value;
    return instance;
}

Napi::Function StructuredOutputConfigWrap::ConstStringWrap::get_class(Napi::Env env) {
    Napi::Function f = DefineClass(env, "ConstString", { 
        InstanceMethod("toString", &ConstStringWrap::to_string)
    });
    ctor = Napi::Persistent(f);
    return f;
}
StructuredOutputConfigWrap::ConstStringWrap::ConstStringWrap(const Napi::CallbackInfo& info)
: Napi::ObjectWrap<ConstStringWrap>(info) {
    OPENVINO_ASSERT(info.Length() == 1, "ConstString constructor requires a single string argument.");
    if (info[0].IsString()) {
        value = ov::genai::StructuredOutputConfig::ConstString(js_to_cpp<std::string>(info.Env(), info[0]));
    }
}
Napi::Object StructuredOutputConfigWrap::ConstStringWrap::wrap(Napi::Env env, const ov::genai::StructuredOutputConfig::ConstString value) {
    auto ctor = StructuredOutputConfigWrap::ConstStringWrap::ctor.Value();
    auto instance = ctor.New({}); 
    auto wrap = Napi::ObjectWrap<StructuredOutputConfigWrap::ConstStringWrap>::Unwrap(instance);
    wrap->value = value;
    return instance;
}

Napi::Function StructuredOutputConfigWrap::AnyTextWrap::get_class(Napi::Env env) {
    Napi::Function f = DefineClass(env, "AnyText", {
        InstanceMethod("toString", &AnyTextWrap::to_string)
    });
    ctor = Napi::Persistent(f);
    return f;
}
StructuredOutputConfigWrap::AnyTextWrap::AnyTextWrap(const Napi::CallbackInfo& info)
: Napi::ObjectWrap<AnyTextWrap>(info) {
    OPENVINO_ASSERT(info.Length() == 0, "AnyText constructor doesn't requires arguments.");
    value = ov::genai::StructuredOutputConfig::AnyText();
}
Napi::Object StructuredOutputConfigWrap::AnyTextWrap::wrap(Napi::Env env, const ov::genai::StructuredOutputConfig::AnyText value) {
    auto ctor = StructuredOutputConfigWrap::AnyTextWrap::ctor.Value();
    auto instance = ctor.New({}); 
    auto wrap = Napi::ObjectWrap<StructuredOutputConfigWrap::AnyTextWrap>::Unwrap(instance);
    wrap->value = value;
    return instance;
}

Napi::Function StructuredOutputConfigWrap::QwenXMLParametersFormatWrap::get_class(Napi::Env env) {
    Napi::Function f = DefineClass(env, "QwenXMLParametersFormat", { 
        InstanceMethod("toString", &QwenXMLParametersFormatWrap::to_string)
    });
    ctor = Napi::Persistent(f);
    return f;
}
StructuredOutputConfigWrap::QwenXMLParametersFormatWrap::QwenXMLParametersFormatWrap(const Napi::CallbackInfo& info)
: Napi::ObjectWrap<QwenXMLParametersFormatWrap>(info) {
    OPENVINO_ASSERT(info.Length() == 1, "QwenXMLParametersFormat constructor requires a single string argument.");
    if (info[0].IsString()) {
        value = ov::genai::StructuredOutputConfig::QwenXMLParametersFormat(js_to_cpp<std::string>(info.Env(), info[0]));
    }
}
Napi::Object StructuredOutputConfigWrap::QwenXMLParametersFormatWrap::wrap(Napi::Env env, const ov::genai::StructuredOutputConfig::QwenXMLParametersFormat value) {
    auto ctor = StructuredOutputConfigWrap::QwenXMLParametersFormatWrap::ctor.Value();
    auto instance = ctor.New({}); 
    auto wrap = Napi::ObjectWrap<StructuredOutputConfigWrap::QwenXMLParametersFormatWrap>::Unwrap(instance);
    wrap->value = value;
    return instance;
}

Napi::Function StructuredOutputConfigWrap::ConcatWrap::get_class(Napi::Env env) {
    Napi::Function f = DefineClass(env, "Concat", {
        InstanceMethod("toString", &ConcatWrap::to_string)
    });
    ctor = Napi::Persistent(f);
    return f;
}
StructuredOutputConfigWrap::ConcatWrap::ConcatWrap(const Napi::CallbackInfo& info)
: Napi::ObjectWrap<ConcatWrap>(info) {
    OPENVINO_ASSERT(info.Length() >= 2, "Concat constructor requires two or more arguments of type StructuralTag.");

    std::vector<ov::genai::StructuredOutputConfig::StructuralTag> elements;
    elements.reserve(info.Length());
    for (uint32_t i = 0; i < info.Length(); ++i) {
        try{
            elements.push_back(js_to_cpp<ov::genai::StructuredOutputConfig::StructuralTag>(info.Env(), info[i]));
        } catch ( ov::Exception& e) {
            OPENVINO_THROW("Concat constructor requires arguments of type StructuralTag, but argument ", i, " has incompatible type.");
        }
    }
    ptr = std::make_shared<ov::genai::StructuredOutputConfig::Concat>(elements);
}
Napi::Object StructuredOutputConfigWrap::ConcatWrap::wrap(Napi::Env env, const std::shared_ptr<ov::genai::StructuredOutputConfig::Concat> value) {
    auto ctor = StructuredOutputConfigWrap::ConcatWrap::ctor.Value();
    auto instance = ctor.New({}); 
    auto wrap = Napi::ObjectWrap<StructuredOutputConfigWrap::ConcatWrap>::Unwrap(instance);
    wrap->ptr = value;
    return instance;
}

Napi::Function StructuredOutputConfigWrap::UnionWrap::get_class(Napi::Env env) {
    Napi::Function f = DefineClass(env, "Union", {
        InstanceMethod("toString", &UnionWrap::to_string)
    });
    ctor = Napi::Persistent(f);
    return f;
}
StructuredOutputConfigWrap::UnionWrap::UnionWrap(const Napi::CallbackInfo& info)
: Napi::ObjectWrap<UnionWrap>(info) {
    OPENVINO_ASSERT(info.Length() >= 2, "Union constructor requires two or more arguments of type StructuralTag.");

    std::vector<ov::genai::StructuredOutputConfig::StructuralTag> elements;
    elements.reserve(info.Length());
    for (uint32_t i = 0; i < info.Length(); ++i) {
        try{
            elements.push_back(js_to_cpp<ov::genai::StructuredOutputConfig::StructuralTag>(info.Env(), info[i]));
        } catch ( ov::Exception& e) {
            OPENVINO_THROW("Union constructor requires arguments of type StructuralTag, but argument ", i, " has incompatible type.");
        }
    }
    ptr = std::make_shared<ov::genai::StructuredOutputConfig::Union>(elements);
}
Napi::Object StructuredOutputConfigWrap::UnionWrap::wrap(Napi::Env env, const std::shared_ptr<ov::genai::StructuredOutputConfig::Union> value) {
    auto ctor = StructuredOutputConfigWrap::UnionWrap::ctor.Value();
    auto instance = ctor.New({}); 
    auto wrap = Napi::ObjectWrap<StructuredOutputConfigWrap::UnionWrap>::Unwrap(instance);
    wrap->ptr = value;
    return instance;
}

Napi::Function StructuredOutputConfigWrap::TagWrap::get_class(Napi::Env env) {
    Napi::Function f = DefineClass(env, "Tag", {
        InstanceMethod("toString", &TagWrap::to_string)
    });
    ctor = Napi::Persistent(f);
    return f;
}
StructuredOutputConfigWrap::TagWrap::TagWrap(const Napi::CallbackInfo& info)
: Napi::ObjectWrap<TagWrap>(info) {
    auto error_message = "Tag constructor requires three arguments: begin (string), content (StructuralTag), end (string).";
    OPENVINO_ASSERT(info.Length() == 3 && info[0].IsString() && info[2].IsString(),error_message);
    ov::genai::StructuredOutputConfig::StructuralTag tag;
    try {
        tag = js_to_cpp<ov::genai::StructuredOutputConfig::StructuralTag>(info.Env(), info[1]);
    } catch ( ov::Exception& e) {
        OPENVINO_THROW(error_message);
    }
    ptr = std::make_shared<ov::genai::StructuredOutputConfig::Tag>(
        js_to_cpp<std::string>(info.Env(), info[0]),
        tag,
        js_to_cpp<std::string>(info.Env(), info[2])
    );
}
Napi::Object StructuredOutputConfigWrap::TagWrap::wrap(Napi::Env env, const std::shared_ptr<ov::genai::StructuredOutputConfig::Tag> value) {
    auto ctor = StructuredOutputConfigWrap::TagWrap::ctor.Value();
    auto instance = ctor.New({}); 
    auto wrap = Napi::ObjectWrap<StructuredOutputConfigWrap::TagWrap>::Unwrap(instance);
    wrap->ptr = value;
    return instance;
}

Napi::Function StructuredOutputConfigWrap::TriggeredTagsWrap::get_class(Napi::Env env) {
    Napi::Function f = DefineClass(env, "TriggeredTags", { 
        InstanceMethod("toString", &TriggeredTagsWrap::to_string) 
    });
    ctor = Napi::Persistent(f);
    return f;
}
StructuredOutputConfigWrap::TriggeredTagsWrap::TriggeredTagsWrap(const Napi::CallbackInfo& info)
: Napi::ObjectWrap<TriggeredTagsWrap>(info) {
    auto error_message = "TriggeredTags constructor requires four arguments: triggers (string[]), tag (StructuralTag), at_least_one (boolean), stop_after_first (boolean).";
    OPENVINO_ASSERT(info.Length() == 4 && info[2].IsBoolean() && info[3].IsBoolean(), error_message);
    std::vector<ov::genai::StructuredOutputConfig::Tag> tags;
    std::vector<std::string> triggers;
    try {
        triggers = js_to_cpp<std::vector<std::string>>(info.Env(), info[0]);
        tags = js_to_cpp<std::vector<ov::genai::StructuredOutputConfig::Tag>>(
            info.Env(),
            info[1]
        );
    } catch ( ov::Exception& e) {
        OPENVINO_THROW(error_message);
    }
    auto p = ov::genai::StructuredOutputConfig::TriggeredTags(
        triggers,
        tags,
        info[2].As<Napi::Boolean>().Value(),
        info[3].As<Napi::Boolean>().Value()
    );
}
Napi::Object StructuredOutputConfigWrap::TriggeredTagsWrap::wrap(Napi::Env env, const std::shared_ptr<ov::genai::StructuredOutputConfig::TriggeredTags> value) {
    auto ctor = StructuredOutputConfigWrap::TriggeredTagsWrap::ctor.Value();
    auto instance = ctor.New({}); 
    auto wrap = Napi::ObjectWrap<StructuredOutputConfigWrap::TriggeredTagsWrap>::Unwrap(instance);
    wrap->ptr = value;
    return instance;
}

Napi::Function StructuredOutputConfigWrap::TagsWithSeparatorWrap::get_class(Napi::Env env) {
    Napi::Function f = DefineClass(env, "TagsWithSeparator", {
        InstanceMethod("toString", &TagsWithSeparatorWrap::to_string)
    });
    ctor = Napi::Persistent(f);
    return f;
}
StructuredOutputConfigWrap::TagsWithSeparatorWrap::TagsWithSeparatorWrap(const Napi::CallbackInfo& info)
: Napi::ObjectWrap<TagsWithSeparatorWrap>(info) {
    auto error_message = "TagsWithSeparator constructor requires 4 arguments.";
    OPENVINO_ASSERT(info.Length() == 4 && info[2].IsString() && info[2].IsBoolean() && info[3].IsBoolean(), error_message);
    std::vector<ov::genai::StructuredOutputConfig::Tag> tags;
    try {
        tags = js_to_cpp<std::vector<ov::genai::StructuredOutputConfig::Tag>>(info.Env(), info[1]);
    } catch ( ov::Exception& e) {
        OPENVINO_THROW(error_message);
    }
    ptr = std::make_shared<ov::genai::StructuredOutputConfig::TagsWithSeparator>(
        tags,
        js_to_cpp<std::string>(info.Env(), info[1]),
        info[2].As<Napi::Boolean>().Value(),
        info[3].As<Napi::Boolean>().Value()
    );
}
Napi::Object StructuredOutputConfigWrap::TagsWithSeparatorWrap::wrap(Napi::Env env, const std::shared_ptr<ov::genai::StructuredOutputConfig::TagsWithSeparator> value) {
    auto ctor = StructuredOutputConfigWrap::TagsWithSeparatorWrap::ctor.Value();
    auto instance = ctor.New({}); 
    auto wrap = Napi::ObjectWrap<StructuredOutputConfigWrap::TagsWithSeparatorWrap>::Unwrap(instance);
    wrap->ptr = value;
    return instance;
}
