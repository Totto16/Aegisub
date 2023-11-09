// from:
// https://github.com/nodejs/node/blob/b6971602564fc93c536ad469947536b487c968ea/test/embedding/embedtest.cc

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "node.h"
#include "uv.h"
#include <assert.h>

// Note: This file is being referred to from doc/api/embedding.md, and excerpts
// from it are included in the documentation. Try to keep these in sync.

using node::CommonEnvironmentSetup;
using node::Environment;
using node::MultiIsolatePlatform;
using v8::Context;
using v8::HandleScope;
using v8::Isolate;
using v8::Locker;
using v8::MaybeLocal;
using v8::V8;
using v8::Value;

static int RunNodeInstance(MultiIsolatePlatform *platform,
                           const std::vector<std::string> &args,
                           const std::vector<std::string> &exec_args);

std::string read_content(const std::string &path) {
  const std::ifstream input_stream(path, std::ios_base::binary);

  if (input_stream.fail()) {
    throw std::runtime_error("Failed to open file");
  }

  std::stringstream buffer;
  buffer << input_stream.rdbuf();

  return buffer.str();
}

int main(int argc, char **argv) {

  if (argc != 2) {
    std::cerr << "exactly 2 args are required!\n";
    return 2;
  }

  std::string file_path = std::string(argv[1]);
  --argc;

  argv = uv_setup_args(argc, argv);
  std::vector<std::string> args(argv, argv + argc);

  args.push_back(file_path);
  args.push_back(read_content(file_path));

  std::unique_ptr<node::InitializationResult> result =
      node::InitializeOncePerProcess(
          args,
          {node::ProcessInitializationFlags::kNoInitializeV8,
           node::ProcessInitializationFlags::kNoInitializeNodeV8Platform});

  for (const std::string &error : result->errors())
    fprintf(stderr, "%s: %s\n", args[0].c_str(), error.c_str());
  if (result->early_return() != 0) {
    return result->exit_code();
  }

  std::unique_ptr<MultiIsolatePlatform> platform =
      MultiIsolatePlatform::Create(4);
  V8::InitializePlatform(platform.get());
  V8::Initialize();

  int ret =
      RunNodeInstance(platform.get(), result->args(), result->exec_args());

  V8::Dispose();
  V8::DisposePlatform();

  node::TearDownOncePerProcess();
  return ret;
}

// Extracts a C string from a V8 Utf8Value.
const char *ToCString(const v8::String::Utf8Value &value) {
  return *value ? *value : "<string conversion failed>";
}

void Print(const v8::FunctionCallbackInfo<v8::Value> &args) {
  bool first = true;
  for (int i = 0; i < args.Length(); i++) {
    v8::HandleScope handle_scope(args.GetIsolate());
    if (first) {
      first = false;
    } else {
      printf(" ");
    }
    v8::String::Utf8Value str(args.GetIsolate(), args[i]);
    const char *cstr = ToCString(str);
    printf("%s", cstr);
  }
  printf("\n");
  fflush(stdout);
}

void addGlobalModules(Isolate *isolate, v8::Local<v8::Context> context) {
  const auto i = context->Global()->Set(
      context,
      v8::String::NewFromUtf8(isolate, "printkli", v8::NewStringType::kNormal)
          .ToLocalChecked(),
      v8::Function::New(context, Print).ToLocalChecked());

  i.Check();
}

int RunNodeInstance(MultiIsolatePlatform *platform,
                    const std::vector<std::string> &args,
                    const std::vector<std::string> &exec_args) {
  int exit_code = 0;

  std::vector<std::string> errors;
  std::unique_ptr<CommonEnvironmentSetup> setup =
      CommonEnvironmentSetup::Create(platform, &errors, args, exec_args);
  if (!setup) {
    for (const std::string &err : errors)
      fprintf(stderr, "%s: %s\n", args[0].c_str(), err.c_str());
    return 1;
  }

  Isolate *isolate = setup->isolate();
  Environment *env = setup->env();

  {
    Locker locker(isolate);
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    auto context = setup->context();
    Context::Scope context_scope(context);

    addGlobalModules(isolate, context);

    MaybeLocal<Value> loadenv_ret = node::LoadEnvironment(
        env, "const module = require('module');"
             "const publicRequire = "
             "module.createRequire(process.cwd() + '/');"
             "globalThis.require = publicRequire;"
             "globalThis.__filename = process.argv[1];"
             "const path = require('path');"
             "globalThis.__dirname = path.dirname(process.argv[1]);"
             "console.log(module.builtinModules);"
             "require('vm').runInThisContext(process.argv[2]);"
             "process.exit(0)"

    );

    if (loadenv_ret.IsEmpty()) // There has been a JS exception.
      return 1;

    exit_code = node::SpinEventLoop(env).FromMaybe(1);

    node::Stop(env);
  }

  return exit_code;
}
