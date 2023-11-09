
#include "v8.h"

#pragma once

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
