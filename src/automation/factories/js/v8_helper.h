#include <concepts>
#include <filesystem>
#include <functional>
#include <node.h>
#include <string>
#include <v8.h>
#include <vector>

#include "auto4_js.h"

#pragma once

namespace Automation4 {

namespace JS {

static int RunNodeInstance(node::MultiIsolatePlatform *platform,
                           const std::vector<std::string> &args,
                           const std::vector<std::string> &exec_args,
                           const std::filesystem::path &path,
                           const bool isModule, const std::uint32_t timeout);

void initAegisubModule(v8::Local<v8::Object> exports);

int execute_js_file(
    const std::string &file_path, const Automation4::JS::JSType type,
    const std::uint32_t timeout,
    std::function<void(std::string, std::string, std::string, std::string)>);

// defines

#define TO_V8(value) to_v8(isolate, value)
#define VECTOR_TO_V8(value) array_from_vector(isolate, value)

// TEMPLATES

// template to_v8

template <class T, class U>
concept Derived = std::is_base_of<U, T>::value;

// template <Derived<v8::Value> T, typename S>
template <typename S>
inline v8::Local<v8::Value> to_v8(v8::Isolate *isolate, S value) {
  static_assert(false, "Not implemented for this type!");
  return isolate->ThrowException(
      v8::Exception::Error(v8::String::NewFromUtf8(isolate, "UNREACHABLE",
                                                   v8::NewStringType::kNormal)
                               .ToLocalChecked()));
}

template <>
inline v8::Local<v8::Value> to_v8<const char *>(v8::Isolate *isolate,
                                                const char *value) {

  return v8::String::NewFromUtf8(isolate, value, v8::NewStringType::kNormal)
      .ToLocalChecked();
}

template <>
inline v8::Local<v8::Value> to_v8<std::string>(v8::Isolate *isolate,
                                               std::string value) {

  return v8::String::NewFromUtf8(isolate, value.c_str(),
                                 v8::NewStringType::kNormal)
      .ToLocalChecked();
}

template <>
inline v8::Local<v8::Value> to_v8<std::uint32_t>(v8::Isolate *isolate,
                                                 std::uint32_t value) {

  return v8::Uint32::NewFromUnsigned(isolate, value);
}

template <>
inline v8::Local<v8::Value> to_v8<bool>(v8::Isolate *isolate, bool value) {

  return v8::Boolean::New(isolate, value);
}

// template array_from_vector
template <typename T>
v8::Local<v8::Array> array_from_vector(v8::Isolate *isolate,
                                       std::vector<T> vec) {

  auto result = v8::Array::New(isolate, vec.size());

  auto context = isolate->GetCurrentContext();
  for (std::uint32_t i = 0; i < vec.size(); ++i) {
    result
        ->Set(context, v8::Uint32::NewFromUnsigned(isolate, i),
              to_v8(isolate, vec.at(i)))
        .Check();
  }

  return result;
}

} // namespace JS
} // namespace Automation4
