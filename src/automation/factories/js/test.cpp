// from:
// https://github.com/nodejs/node/blob/b6971602564fc93c536ad469947536b487c968ea/test/embedding/embedtest.cc

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "node.h"
#include "test.hpp"
#include "uv.h"

// in milliseconds!
#define SCRIPT_EXECUTION_TIMEOUT_MS (60 * 1000)

// Note: This file is being referred to from doc/api/embedding.md, and excerpts
// from it are included in the documentation. Try to keep these in sync.

static int RunNodeInstance(node::MultiIsolatePlatform *platform,
                           const std::vector<std::string> &args,
                           const std::vector<std::string> &exec_args,
                           const std::filesystem::path &path,
                           const bool isModule);

std::string read_content(const std::string &path) {
  const std::ifstream input_stream(path, std::ios_base::binary);

  if (input_stream.fail()) {
    throw std::runtime_error("Failed to open file");
  }

  std::stringstream buffer;
  buffer << input_stream.rdbuf();

  return buffer.str();
}

void Init2(v8::Local<v8::Object> exports);

int main(int argc, char **argv) {

  if (argc != 2) {
    std::cerr << "exactly 2 args are required!\n";
    return 2;
  }

  std::string file_path = std::string(argv[1]);

  // NODE_MODULE_LINKED(aegisub, Init2);

  node::node_module _module = {
      18,
      node::ModuleFlags::kLinked,
      NULL, /* NOLINT (readability/null_usage) */
      __FILE__,
      NULL, /* NOLINT (readability/null_usage) */
      (node::addon_context_register_func)(Init2),
      "aegisub",
      NULL,
      NULL /* NOLINT (readability/null_usage) */
  };

  node_module_register(&_module);

  std::vector<std::string> args{};
  args.emplace_back(
      "aegisub embedded node.js"); // gets replaceed by actual binary by
                                   // InitializeOncePerProcess
  args.push_back(file_path);

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

  std::unique_ptr<node::MultiIsolatePlatform> platform =
      node::MultiIsolatePlatform::Create(4);
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();

  // TODO: use aegisub detection!;
  const bool isModule = std::filesystem::path(file_path).extension() == ".mjs";
  auto exec_args = result->exec_args();
  if (isModule) {
    // TODO : get this to work
    exec_args.push_back("--experimental-vm-modules");
  }

  int ret = RunNodeInstance(platform.get(), result->args(), exec_args,
                            file_path, isModule);

  v8::V8::Dispose();
  v8::V8::DisposePlatform();

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
  printf(" <- c++\n");
  fflush(stdout);
}

void Print2(const v8::FunctionCallbackInfo<v8::Value> &args) {
  printf("C++ print2\n");
  fflush(stdout);
}

void Init2(v8::Local<v8::Object> exports) {
  printf("INIT\n");
  v8::Local<v8::Context> context =
      exports->GetCreationContext().ToLocalChecked();
  auto isolate = context->GetIsolate();
  exports
      ->Set(context,
            v8::String::NewFromUtf8(isolate, "printkli",
                                    v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::Function::New(context, Print).ToLocalChecked())
      .Check();
}

v8::MaybeLocal<v8::Value>
AegisubModuleEvaluationSteps(v8::Local<v8::Context> context,
                             v8::Local<v8::Module> module) {

  auto isolate = context->GetIsolate();
  module
      ->SetSyntheticModuleExport(
          isolate,
          v8::String::NewFromUtf8(isolate, "printkli",
                                  v8::NewStringType::kNormal)
              .ToLocalChecked(),
          v8::Function::New(context, Print).ToLocalChecked())
      .Check();

  return v8::MaybeLocal<v8::Value>(v8::Null(isolate));
}

void addGlobalProperties(v8::Isolate *isolate, v8::Local<v8::Context> &context,
                         const std::filesystem::path &path,
                         const std::uint32_t timeout, const bool isModule) {
  context->Global()
      ->Set(context,
            v8::String::NewFromUtf8(isolate, "printkli",
                                    v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::Function::New(context, Print).ToLocalChecked())
      .Check();

  // https://nodejs.org/docs/latest-v18.x/api/esm.html#no-__filename-or-__dirname
  if (!isModule) {

    context->Global()
        ->Set(context,
              v8::String::NewFromUtf8(isolate, "__filename",
                                      v8::NewStringType::kNormal)
                  .ToLocalChecked(),
              v8::String::NewFromUtf8(isolate, path.string().c_str(),
                                      v8::NewStringType::kNormal)
                  .ToLocalChecked())
        .Check();

    context->Global()
        ->Set(context,
              v8::String::NewFromUtf8(isolate, "__dirname",
                                      v8::NewStringType::kNormal)
                  .ToLocalChecked(),
              v8::String::NewFromUtf8(isolate,
                                      path.parent_path().string().c_str(),
                                      v8::NewStringType::kNormal)
                  .ToLocalChecked())
        .Check();
  }

  v8::Local<v8::Module> module = v8::Module::CreateSyntheticModule(
      isolate,
      v8::String::NewFromUtf8(isolate, "aegisub", v8::NewStringType::kNormal)
          .ToLocalChecked(),
      {v8::String::NewFromUtf8(isolate, "aegisub", v8::NewStringType::kNormal)
           .ToLocalChecked(),
       v8::String::NewFromUtf8(isolate, "node:aegisub",
                               v8::NewStringType::kNormal)
           .ToLocalChecked()},
      AegisubModuleEvaluationSteps);

  v8::Local<v8::Object> internalProperties = v8::Object::New(isolate);
  internalProperties
      ->Set(context,
            v8::String::NewFromUtf8(isolate, "fileContent",
                                    v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::String::NewFromUtf8(isolate, read_content(path).c_str(),
                                    v8::NewStringType::kNormal)
                .ToLocalChecked())
      .Check();

  internalProperties
      ->Set(context,
            v8::String::NewFromUtf8(isolate, "timeout",
                                    v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::Uint32::NewFromUnsigned(isolate, timeout))
      .Check();

  internalProperties
      ->Set(context,
            v8::String::NewFromUtf8(isolate, "isModule",
                                    v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::Boolean::New(isolate, isModule))
      .Check();

  internalProperties
      ->Set(context,
            v8::String::NewFromUtf8(isolate, "filename",
                                    v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::String::NewFromUtf8(isolate, path.string().c_str(),
                                    v8::NewStringType::kNormal)
                .ToLocalChecked())
      .Check();

  std::vector<v8::Local<v8::String>> values{
      v8::String::NewFromUtf8(isolate, "aegisub", v8::NewStringType::kNormal)
          .ToLocalChecked()};

  v8::Local<v8::Array> linkedBindingsArray = array_from_vector(isolate, values);

  internalProperties
      ->Set(context,
            v8::String::NewFromUtf8(isolate, "linkedBindings",
                                    v8::NewStringType::kNormal)
                .ToLocalChecked(),
            linkedBindingsArray)
      .Check();

  context->Global()
      ->Set(context,
            v8::String::NewFromUtf8(isolate, "__internal__properties__",
                                    v8::NewStringType::kNormal)
                .ToLocalChecked(),
            internalProperties)
      .Check();

  if (isModule) {
    isolate->SetHostImportModuleDynamicallyCallback(
        [](v8::Local<v8::Context> context,
           v8::Local<v8::Data> host_defined_options,
           v8::Local<v8::Value> resource_name, v8::Local<v8::String> specifier,
           v8::Local<v8::FixedArray> import_assertions)
            -> v8::MaybeLocal<v8::Promise> {
          printf("test a\n");
          return v8::MaybeLocal<v8::Promise>();
        });

    isolate->SetHostInitializeImportMetaObjectCallback(
        [](v8::Local<v8::Context> context, v8::Local<v8::Module> module,
           v8::Local<v8::Object> meta) -> void { printf("test b\n"); });
  }
}

int RunNodeInstance(node::MultiIsolatePlatform *platform,
                    const std::vector<std::string> &args,
                    const std::vector<std::string> &exec_args,
                    const std::filesystem::path &path, const bool isModule) {
  int exit_code = 0;

  std::vector<std::string> errors;
  std::unique_ptr<node::CommonEnvironmentSetup> setup =
      node::CommonEnvironmentSetup::Create(platform, &errors, args, exec_args);
  if (!setup) {
    for (const std::string &err : errors)
      std::cerr << args[0] << ": " << err << "\n";
    return 1;
  }

  v8::Isolate *isolate = setup->isolate();
  node::Environment *env = setup->env();

  {
    v8::Locker locker(isolate);
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    auto context = setup->context();
    v8::Context::Scope context_scope(context);

    addGlobalProperties(isolate, context, path, SCRIPT_EXECUTION_TIMEOUT_MS,
                        isModule);

    v8::MaybeLocal<v8::Value> loadenv_ret = node::LoadEnvironment(
        env,
        "const {fileContent, timeout, linkedBindings, filename, isModule} = "
        "globalThis.__internal__properties__;\n"
        "delete globalThis.__internal__properties__;\n"
        "const _vm = require('vm');\n"
        "if(isModule){\n"
        "console.log('is Module');\n"
        "}else{\n"
        "console.log('is CommonJS');\n"
        "const publicRequire = "
        "require('module').createRequire(process.cwd() + '/');\n"
        "const modifiedRequire = (name)=>{\n"
        "if (linkedBindings.includes(name)) {\n"
        "return process._linkedBinding(name)\n"
        "}\n"
        "return publicRequire(name);\n"
        "};\n"
        "globalThis.require = modifiedRequire;\n"
        "}\n"

        //  "console.log(module.builtinModules);"

        "const script = "
        "new _vm.Script(fileContent,{filename: filename});\n"
        // runInThisContext has only access to globals (globalThis), but
        // not locals!
        "script.runInThisContext({displayErrors:true, "
        "timeout: timeout});\n" // after running for 'timeout'
                                // milli-seconds the script gets killed!
        "process.exit(0);\n"

    );

    if (loadenv_ret.IsEmpty()) // There has been a JS exception.
      return 1;

    exit_code = node::SpinEventLoop(env).FromMaybe(1);

    node::Stop(env);
  }

  return exit_code;
}
