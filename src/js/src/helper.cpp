#include "include/helper.hpp"
#include "include/addon.hpp"
#include "include/structured_output_config.hpp"

namespace {
constexpr const char* JS_SCHEDULER_CONFIG_KEY = "schedulerConfig";
constexpr const char* CPP_SCHEDULER_CONFIG_KEY = "scheduler_config";
constexpr const char* POOLING_TYPE_KEY = "pooling_type";
constexpr const char* STRUCTURED_OUTPUT_CONFIG_KEY = "structured_output_config";
constexpr const char* STRUCTURAL_TAGS_CONFIG_KEY = "structural_tags_config";
constexpr const char* STRUCTURAL_TAGS_KEY = "structural_tags";
constexpr const char* COMPOUND_GRAMMAR_KEY = "compound_grammar";
}  // namespace

ov::AnyMap to_anyMap(const Napi::Env& env, const Napi::Value& val) {
    ov::AnyMap properties;
    if (!val.IsObject()) {
        OPENVINO_THROW("Passed Napi::Value must be an object.");
    }
    const auto& parameters = val.ToObject();
    const auto& keys = parameters.GetPropertyNames();

    for (uint32_t i = 0; i < keys.Length(); ++i) {
        const auto& property_name = static_cast<Napi::Value>(keys[i]).ToString().Utf8Value();

        const auto& any_value = js_to_cpp<ov::Any>(env, parameters.Get(property_name));

        properties.insert(std::make_pair(property_name, any_value));
    }

    return properties;
}

template <>
ov::Any js_to_cpp<ov::Any>(const Napi::Env& env, const Napi::Value& value) {
    if (value.IsString()) {
        return ov::Any(value.ToString().Utf8Value());
    } else if (value.IsBigInt()) {
        Napi::BigInt big_value = value.As<Napi::BigInt>();
        bool is_lossless;
        int64_t big_num = big_value.Int64Value(&is_lossless);

        if (!is_lossless) {
            OPENVINO_THROW("Result of BigInt conversion to int64_t results in a loss of precision");
        }

        return ov::Any(big_num);
    } else if (value.IsNumber()) {
        Napi::Number num = value.ToNumber();

        if (is_napi_value_int(env, value)) {
            return ov::Any(num.Int32Value());
        } else {
            return ov::Any(num.DoubleValue());
        }
    } else if (value.IsBoolean()) {
        return ov::Any(static_cast<bool>(value.ToBoolean()));
    } else if (value.IsArray()) {
        return ov::Any(js_to_cpp<std::vector<std::string>>(env, value));
    } else if (value.IsObject()) {
        if (value.ToString().Utf8Value() == "[object Set]") {
            try {
                // try to cast to set of strings
                auto object_value = value.As<Napi::Object>();
                auto values = object_value.Get("values").As<Napi::Function>();
                auto iterator = values.Call(object_value, {}).As<Napi::Object>();
                auto next = iterator.Get("next").As<Napi::Function>();
                auto size = object_value.Get("size").As<Napi::Number>().Int32Value();

                std::set<std::string> set;
                for (uint32_t i = 0; i < size; ++i) {
                    auto item = next.Call(iterator, {}).As<Napi::Object>();
                    set.insert(item.Get("value").As<Napi::String>().Utf8Value());
                }

                return ov::Any(set);
            } catch (std::exception& e) {
                std::cerr << "Cannot convert to set: " << e.what() << std::endl;
            }
        } else {
            return js_to_cpp<ov::AnyMap>(env, value);
        }
    }
    OPENVINO_THROW("Cannot convert " + value.ToString().Utf8Value() + " to ov::Any");
}

template <>
ov::AnyMap js_to_cpp<ov::AnyMap>(const Napi::Env& env, const Napi::Value& value) {
    std::map<std::string, ov::Any> result_map;
    if(value.IsUndefined() || value.IsNull()) {
        return result_map;
    }
    if (!value.IsObject()) {
        OPENVINO_THROW("Passed Napi::Value must be an object.");
    }
    const auto& object = value.ToObject();
    const auto& keys = object.GetPropertyNames();

    for (uint32_t i = 0; i < keys.Length(); ++i) {
        const std::string& key_name = keys.Get(i).ToString();
        auto value_by_key = object.Get(key_name);
        if (value_by_key.IsUndefined() || value_by_key.IsNull()) {
            continue;
        }
        if (key_name == JS_SCHEDULER_CONFIG_KEY) {
            result_map[CPP_SCHEDULER_CONFIG_KEY] = js_to_cpp<ov::genai::SchedulerConfig>(env, value_by_key);
        } else if (key_name == POOLING_TYPE_KEY) {
            result_map[key_name] =
                ov::genai::TextEmbeddingPipeline::PoolingType(value_by_key.ToNumber().Int32Value());
        // } else if (is_structured_output_config(env, value_by_key)) {
        //     std::cout << "Unwrapping StructuredOutputConfig for key: " << key_name << std::endl;
        //     result_map[key_name] = *Napi::ObjectWrap<StructuredOutputConfigWrap>::Unwrap(value_by_key.ToObject())->get_value();
        } else if (key_name == STRUCTURED_OUTPUT_CONFIG_KEY) {
            std::cout << "Unwrapping StructuredOutputConfig for key: " << key_name << std::endl;
            auto c = *Napi::ObjectWrap<StructuredOutputConfigWrap>::Unwrap(value_by_key.ToObject())->get_value();
            result_map[key_name] = c;
            std::cout << "!!! " << c.json_schema.value_or("NONE") << std::endl;
            // result_map[key_name] = ov::genai::StructuredOutputConfig(js_to_cpp<ov::AnyMap>(env, value_by_key));
        // } else if (key_name == STRUCTURAL_TAGS_CONFIG_KEY) {
        //     result_map[key_name] = ov::genai::StructuralTagsConfig(js_to_cpp<ov::AnyMap>(env, value_by_key));
        // } else if (key_name == STRUCTURAL_TAGS_KEY) {
        //     result_map[key_name] = js_to_cpp<std::vector<ov::genai::StructuralTagItem>>(env, value_by_key);
        // } else if (key_name == COMPOUND_GRAMMAR_KEY) {
        //     result_map[key_name] = js_to_cpp<ov::genai::StructuredOutputConfig::CompoundGrammar>(env, value_by_key);
        } else {
            std::cout << "Unmatched for key: " << key_name << std::endl;
            result_map[key_name] = js_to_cpp<ov::Any>(env, value_by_key);
        }
    }

    return result_map;
}

template <>
std::string js_to_cpp<std::string>(const Napi::Env& env, const Napi::Value& value) {
    if (value.IsString()) {
        return value.As<Napi::String>().Utf8Value();
    } else {
        OPENVINO_THROW("Passed argument must be of type String.");
    }
}

template <>
std::vector<std::string> js_to_cpp<std::vector<std::string>>(const Napi::Env& env, const Napi::Value& value) {
    if (value.IsArray()) {
        auto array = value.As<Napi::Array>();
        size_t arrayLength = array.Length();

        std::vector<std::string> nativeArray;
        for (uint32_t i = 0; i < arrayLength; ++i) {
            Napi::Value arrayItem = array[i];
            if (!arrayItem.IsString()) {
                OPENVINO_THROW(std::string("Passed array must contain only strings."));
            }
            nativeArray.push_back(arrayItem.As<Napi::String>().Utf8Value());
        }
        return nativeArray;

    } else {
        OPENVINO_THROW("Passed argument must be of type Array or TypedArray.");
    }
}

template <>
std::vector<ov::genai::StructuralTagItem> js_to_cpp<std::vector<ov::genai::StructuralTagItem>>(const Napi::Env& env, const Napi::Value& value) {
    if (value.IsArray()) {
        auto array = value.As<Napi::Array>();
        size_t arrayLength = array.Length();

        std::vector<ov::genai::StructuralTagItem> nativeArray;
        for (uint32_t i = 0; i < arrayLength; ++i) {
            Napi::Value arrayItem = array[i];
            if (!arrayItem.IsObject()) {
                OPENVINO_THROW(std::string("Passed array must contain only objects."));
            }
            nativeArray.push_back(ov::genai::StructuralTagItem(js_to_cpp<ov::AnyMap>(env, arrayItem)));
        }
        return nativeArray;

    } else {
        OPENVINO_THROW("Passed argument must be of type Array or TypedArray.");
    }
}

// template <>
// ov::genai::StructuredOutputConfig::CompoundGrammar js_to_cpp<ov::genai::StructuredOutputConfig::CompoundGrammar>(const Napi::Env& env, const Napi::Value& value) {
//     OPENVINO_ASSERT(value.IsObject(), "CompoundGrammar must be a JS object");
//     auto obj = value.As<Napi::Object>();

//     if (obj.Has("json_schema")) {
//         return ov::genai::StructuredOutputConfig::JSONSchema(
//             js_to_cpp<std::string>(env, obj.Get("json_schema"))
//         );
//     }
//     if (obj.Has("regex")) {
//         return ov::genai::StructuredOutputConfig::Regex(
//             js_to_cpp<std::string>(env, obj.Get("regex"))
//         );
//     }
//     if (obj.Has("grammar")) {
//         return ov::genai::StructuredOutputConfig::EBNF(
//             js_to_cpp<std::string>(env, obj.Get("grammar"))
//         );
//     }
//     if (obj.Has("compoundType") && obj.Has("left") && obj.Has("right")) {
//         auto left = js_to_cpp<ov::genai::StructuredOutputConfig::CompoundGrammar>(env, obj.Get("left"));
//         auto right = js_to_cpp<ov::genai::StructuredOutputConfig::CompoundGrammar>(env, obj.Get("right"));
//         auto compound_type = obj.Get("compoundType").ToString().Utf8Value();
//         if (compound_type == "Concat") {
//             return std::make_shared<ov::genai::StructuredOutputConfig::Concat>(left, right);
//         } else if (compound_type == "Union") {
//             return std::make_shared<ov::genai::StructuredOutputConfig::Union>(left, right);
//         } else {
//             OPENVINO_THROW("compoundType must be either 'Concat' or 'Union'");
//         }
        
//     }
//     OPENVINO_THROW("CompoundGrammar must be either JSONSchema, Regex, EBNF, Concat or Union");
// }

template <>
ov::genai::StringInputs js_to_cpp<ov::genai::StringInputs>(const Napi::Env& env, const Napi::Value& value) {
    if (value.IsString()) {
        return value.As<Napi::String>().Utf8Value();
    } else if (value.IsArray()) {
        return js_to_cpp<std::vector<std::string>>(env, value);
    } else {
        OPENVINO_THROW("Passed argument must be a string or an array of strings");
    }
}

template <>
ov::genai::ChatHistory js_to_cpp<ov::genai::ChatHistory>(const Napi::Env& env, const Napi::Value& value) {
    // TODO Update for new ChatHistory type: Record<string, any>[]
    auto incorrect_argument_message = "Chat history must be { role: string, content: string }[]";
    if (value.IsArray()) {
        auto array = value.As<Napi::Array>();
        size_t arrayLength = array.Length();

        std::vector<ov::AnyMap> nativeArray;
        for (uint32_t i = 0; i < arrayLength; ++i) {
            Napi::Value arrayItem = array[i];
            if (!arrayItem.IsObject()) {
                OPENVINO_THROW(incorrect_argument_message);
            }
            auto obj = arrayItem.As<Napi::Object>();
            if (obj.Get("role").IsUndefined() || obj.Get("content").IsUndefined()) {
                OPENVINO_THROW(incorrect_argument_message);
            }
            ov::AnyMap result;
            Napi::Array keys = obj.GetPropertyNames();

            for (uint32_t i = 0; i < keys.Length(); ++i) {
                Napi::Value key = keys[i];
                Napi::Value value = obj.Get(key);

                std::string keyStr = key.ToString().Utf8Value();
                std::string valueStr = value.ToString().Utf8Value();

                result[keyStr] = valueStr;
            }
            nativeArray.push_back(result);
        }
        return nativeArray;

    } else {
        OPENVINO_THROW(incorrect_argument_message);
    }
}

template <>
ov::genai::SchedulerConfig js_to_cpp<ov::genai::SchedulerConfig>(const Napi::Env& env, const Napi::Value& value) {
    ov::genai::SchedulerConfig config;
    OPENVINO_ASSERT(value.IsObject(), "SchedulerConfig must be a JS object");
    auto obj = value.As<Napi::Object>();

    if (obj.Has("max_num_batched_tokens")) {
        config.max_num_batched_tokens = obj.Get("max_num_batched_tokens").ToNumber().Uint32Value();
    }
    if (obj.Has("num_kv_blocks")) {
        config.num_kv_blocks = obj.Get("num_kv_blocks").ToNumber().Uint32Value();
    }
    if (obj.Has("cache_size")) {
        config.cache_size = obj.Get("cache_size").ToNumber().Uint32Value();
    }
    if (obj.Has("dynamic_split_fuse")) {
        config.dynamic_split_fuse = obj.Get("dynamic_split_fuse").ToBoolean().Value();
    }

    return config;
}

/** @brief  A template specialization for TargetType ov::genai::StructuredOutputConfig::StructuralTag */
template <>
ov::genai::StructuredOutputConfig::StructuralTag js_to_cpp<ov::genai::StructuredOutputConfig::StructuralTag>(const Napi::Env& env, const Napi::Value& value) {
    if (value.IsString()) {
        return js_to_cpp<std::string>(env, value);
    }
    
    OPENVINO_ASSERT(value.IsObject(), "StructuralTag must be a JS object or string");
    auto object = value.As<Napi::Object>();

    if (object.InstanceOf(StructuredOutputConfigWrap::RegexWrap::ctor.Value()))
        return Napi::ObjectWrap<StructuredOutputConfigWrap::RegexWrap>::Unwrap(object)->get_value();
    if (object.InstanceOf(StructuredOutputConfigWrap::JSONSchemaWrap::ctor.Value()))
        return Napi::ObjectWrap<StructuredOutputConfigWrap::JSONSchemaWrap>::Unwrap(object)->get_value();
    if (object.InstanceOf(StructuredOutputConfigWrap::EBNFWrap::ctor.Value()))
        return Napi::ObjectWrap<StructuredOutputConfigWrap::EBNFWrap>::Unwrap(object)->get_value();
    if (object.InstanceOf(StructuredOutputConfigWrap::ConstStringWrap::ctor.Value()))
        return Napi::ObjectWrap<StructuredOutputConfigWrap::ConstStringWrap>::Unwrap(object)->get_value();
    if (object.InstanceOf(StructuredOutputConfigWrap::AnyTextWrap::ctor.Value()))
        return Napi::ObjectWrap<StructuredOutputConfigWrap::AnyTextWrap>::Unwrap(object)->get_value();
    if (object.InstanceOf(StructuredOutputConfigWrap::QwenXMLParametersFormatWrap::ctor.Value()))
        return Napi::ObjectWrap<StructuredOutputConfigWrap::QwenXMLParametersFormatWrap>::Unwrap(object)->get_value();
    if (object.InstanceOf(StructuredOutputConfigWrap::ConcatWrap::ctor.Value()))
        return Napi::ObjectWrap<StructuredOutputConfigWrap::ConcatWrap>::Unwrap(object)->get_value();
    if (object.InstanceOf(StructuredOutputConfigWrap::UnionWrap::ctor.Value()))
        return Napi::ObjectWrap<StructuredOutputConfigWrap::UnionWrap>::Unwrap(object)->get_value();
    if (object.InstanceOf(StructuredOutputConfigWrap::TagWrap::ctor.Value()))
        return Napi::ObjectWrap<StructuredOutputConfigWrap::TagWrap>::Unwrap(object)->get_value();
    if (object.InstanceOf(StructuredOutputConfigWrap::TriggeredTagsWrap::ctor.Value()))
        return Napi::ObjectWrap<StructuredOutputConfigWrap::TriggeredTagsWrap>::Unwrap(object)->get_value();
    if (object.InstanceOf(StructuredOutputConfigWrap::TagsWithSeparatorWrap::ctor.Value()))
        return Napi::ObjectWrap<StructuredOutputConfigWrap::TagsWithSeparatorWrap>::Unwrap(object)->get_value();

    OPENVINO_THROW("Invalid value for StructuralTag.");
}

/** @brief  A template specialization for TargetType std::vector<ov::genai::StructuredOutputConfig::Tag> */
template <>
std::vector<ov::genai::StructuredOutputConfig::Tag> js_to_cpp<std::vector<ov::genai::StructuredOutputConfig::Tag>>(const Napi::Env& env, const Napi::Value& value) {
    OPENVINO_ASSERT(value.IsArray(), "Tags must be a array of StructuredOutputConfig.Tag");
    auto array = value.As<Napi::Array>();
    std::vector<ov::genai::StructuredOutputConfig::Tag> tags;
    tags.reserve(array.Length());

    for (size_t i = 0; i < array.Length(); ++i) {
        auto item = array.Get(i);
        OPENVINO_ASSERT(item.IsObject(), "Each Tag must be a JS object");
        auto object = item.As<Napi::Object>();
        if (object.InstanceOf(StructuredOutputConfigWrap::TagWrap::ctor.Value())) {
            tags.push_back(*Napi::ObjectWrap<StructuredOutputConfigWrap::TagWrap>::Unwrap(object)->get_value());
        } else {
            OPENVINO_THROW("Invalid value for Tag.");
        }
    }
    return tags;
}

template <>
Napi::Value cpp_to_js<ov::genai::EmbeddingResult, Napi::Value>(const Napi::Env& env,
                                                               const ov::genai::EmbeddingResult embedding_result) {
    return std::visit(overloaded{[env](std::vector<float> embed_vector) -> Napi::Value {
                                     auto vector_size = embed_vector.size();
                                     auto buffer = Napi::ArrayBuffer::New(env, vector_size * sizeof(float));
                                     std::memcpy(buffer.Data(), embed_vector.data(), vector_size * sizeof(float));
                                     Napi::Value typed_array = Napi::Float32Array::New(env, vector_size, buffer, 0);
                                     return typed_array;
                                 },
                                 [env](std::vector<int8_t> embed_vector) -> Napi::Value {
                                     auto buffer_size = embed_vector.size();
                                     auto buffer = Napi::ArrayBuffer::New(env, buffer_size * sizeof(int8_t));
                                     std::memcpy(buffer.Data(), embed_vector.data(), buffer_size * sizeof(int8_t));
                                     Napi::Value typed_array = Napi::Int8Array::New(env, buffer_size, buffer, 0);
                                     return typed_array;
                                 },
                                 [env](std::vector<uint8_t> embed_vector) -> Napi::Value {
                                     auto buffer_size = embed_vector.size();
                                     auto buffer = Napi::ArrayBuffer::New(env, buffer_size * sizeof(uint8_t));
                                     std::memcpy(buffer.Data(), embed_vector.data(), buffer_size * sizeof(uint8_t));
                                     Napi::Value typed_array = Napi::Uint8Array::New(env, buffer_size, buffer, 0);
                                     return typed_array;
                                 },
                                 [env](auto& args) -> Napi::Value {
                                     OPENVINO_THROW("Unsupported type for EmbeddingResult.");
                                 }},
                      embedding_result);
}

template <>
Napi::Value cpp_to_js<ov::genai::EmbeddingResults, Napi::Value>(const Napi::Env& env,
                                                                const ov::genai::EmbeddingResults embedding_result) {
    return std::visit(
        [env](auto& embed_vector) {
            auto js_result = Napi::Array::New(env, embed_vector.size());
            for (auto i = 0; i < embed_vector.size(); i++) {
                js_result[i] = cpp_to_js<ov::genai::EmbeddingResult, Napi::Value>(env, embed_vector[i]);
            }
            return js_result;
        },
        embedding_result);
}

template <>
Napi::Value cpp_to_js<std::vector<std::string>, Napi::Value>(const Napi::Env& env,
                                                             const std::vector<std::string> value) {
    auto js_array = Napi::Array::New(env, value.size());
    for (auto i = 0; i < value.size(); i++) {
        js_array[i] = Napi::String::New(env, value[i]);
    }
    return js_array;
}

template <>
Napi::Value cpp_to_js<std::vector<float>, Napi::Value>(const Napi::Env& env, const std::vector<float> value) {
    auto js_array = Napi::Array::New(env, value.size());
    for (auto i = 0; i < value.size(); i++) {
        js_array[i] = Napi::Number::New(env, value[i]);
    }
    return js_array;
}

template <>
Napi::Value cpp_to_js<std::vector<double>, Napi::Value>(const Napi::Env& env, const std::vector<double> value) {
    auto js_array = Napi::Array::New(env, value.size());
    for (auto i = 0; i < value.size(); i++) {
        js_array[i] = Napi::Number::New(env, value[i]);
    }
    return js_array;
}

template <>
Napi::Value cpp_to_js<std::vector<size_t>, Napi::Value>(const Napi::Env& env, const std::vector<size_t> value) {
    auto js_array = Napi::Array::New(env, value.size());
    for (auto i = 0; i < value.size(); i++) {
        js_array[i] = Napi::Number::New(env, value[i]);
    }
    return js_array;
}

template <>
Napi::Value cpp_to_js<ov::genai::StructuredOutputConfig::StructuralTag, Napi::Value>(const Napi::Env& env,
    const ov::genai::StructuredOutputConfig::StructuralTag value
) {
    return std::visit(
        overloaded{
            [env](const std::string& str) -> Napi::Value {
                return Napi::String::New(env, str);
            },
            [env](const ov::genai::StructuredOutputConfig::Regex& regex) -> Napi::Value {
                return StructuredOutputConfigWrap::RegexWrap::wrap(env, regex);
            },
            [env](const ov::genai::StructuredOutputConfig::JSONSchema& json_schema) -> Napi::Value {
                return StructuredOutputConfigWrap::JSONSchemaWrap::wrap(env, json_schema);
            },
            [env](const ov::genai::StructuredOutputConfig::EBNF& ebnf) -> Napi::Value {
                return StructuredOutputConfigWrap::EBNFWrap::wrap(env, ebnf);
            },
            [env](const ov::genai::StructuredOutputConfig::ConstString& const_string) -> Napi::Value {
                return StructuredOutputConfigWrap::ConstStringWrap::wrap(env, const_string);
            },
            [env](const ov::genai::StructuredOutputConfig::AnyText& any_text) -> Napi::Value {
                return StructuredOutputConfigWrap::AnyTextWrap::wrap(env, any_text);
            },
            [env](const ov::genai::StructuredOutputConfig::QwenXMLParametersFormat& qwen_xml) -> Napi::Value {
                return StructuredOutputConfigWrap::QwenXMLParametersFormatWrap::wrap(env, qwen_xml);
            },
            [env](const std::shared_ptr<ov::genai::StructuredOutputConfig::Concat>& concat) -> Napi::Value {
                return StructuredOutputConfigWrap::ConcatWrap::wrap(env, concat);
            },
            [env](const std::shared_ptr<ov::genai::StructuredOutputConfig::Union>& union_value) -> Napi::Value {
                return StructuredOutputConfigWrap::UnionWrap::wrap(env, union_value);
            },
            [env](const std::shared_ptr<ov::genai::StructuredOutputConfig::Tag>& tag) -> Napi::Value {
                return StructuredOutputConfigWrap::TagWrap::wrap(env, tag);
            },
            [env](const std::shared_ptr<ov::genai::StructuredOutputConfig::TriggeredTags>& triggered_tags) -> Napi::Value {
                return StructuredOutputConfigWrap::TriggeredTagsWrap::wrap(env, triggered_tags);
            },
            [env](const std::shared_ptr<ov::genai::StructuredOutputConfig::TagsWithSeparator>& tags_with_separator) -> Napi::Value {
                return StructuredOutputConfigWrap::TagsWithSeparatorWrap::wrap(env, tags_with_separator);
            }
        },
        value);
}

bool is_napi_value_int(const Napi::Env& env, const Napi::Value& num) {
    return env.Global().Get("Number").ToObject().Get("isInteger").As<Napi::Function>().Call({num}).ToBoolean().Value();
}

bool is_structured_output_config(const Napi::Env& env, const Napi::Value& value) {
    if (!value.IsObject()) {
        std::cout << value.ToString() << " is not an Object" << std::endl;
        return false;
    }
    std::cout << "is Object" << std::endl;
    std::cout << value.ToObject().ToString() << " is Object" << std::endl;
    const auto& prototype =  env.GetInstanceData<AddonData>()->structured_output_config;
    return value.ToObject().InstanceOf(prototype.Value().As<Napi::Function>());
}
