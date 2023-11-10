#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "node.h"
#include "v8.h"

#pragma once

static int RunNodeInstance(node::MultiIsolatePlatform *platform,
                           const std::vector<std::string> &args,
                           const std::vector<std::string> &exec_args,
                           const std::filesystem::path &path,
                           const bool isModule, const std::uint32_t timeout);

void Init2(v8::Local<v8::Object> exports);

int execute_js_file(
    const std::string &file_path, const Automation4::JSType type,
    const std::uint32_t timeout,
    std::function<void(std::string, std::string, std::string, std::string)>);

template <typename T>
v8::Local<v8::Array> array_from_vector(v8::Isolate *isolate,
                                       std::vector<T> vec) {

  auto result = v8::Array::New(isolate, vec.size());

  auto context = isolate->GetCurrentContext();
  for (std::uint32_t i = 0; i < vec.size(); ++i) {
    result->Set(context, v8::Uint32::NewFromUnsigned(isolate, i), vec.at(i))
        .Check();
  }

  return result;
}
