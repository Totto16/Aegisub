#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "auto4_js.h"
#include "convertable.h"
#include "idl_headers.h"
#include "node.h"
#include "uv.h"
#include "v8_helper.h"

using namespace Automation4::JS;

namespace Automation4 {

namespace JS {

std::string read_content(const std::string &path) {
  const std::ifstream input_stream(path, std::ios_base::binary);

  if (input_stream.fail()) {
    throw std::runtime_error("Failed to open file");
  }

  std::stringstream buffer;
  buffer << input_stream.rdbuf();

  return buffer.str();
}

int execute_js_file(
    const std::string &file_path, const JSType type,
    const std::uint32_t timeout,
    std::function<void(std::string, std::string, std::string, std::string)>
        callback) {

  // NODE_MODULE_LINKED(aegisub, initAegisubModule);
  node::node_module _module = {
      18,        node::ModuleFlags::kLinked,
      NULL,      __FILE__,
      NULL,      (node::addon_context_register_func)(initAegisubModule),
      "aegisub", NULL,
      NULL};

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
                            file_path, isModule, timeout);

  v8::V8::Dispose();
  v8::V8::DisposePlatform();

  node::TearDownOncePerProcess();
  return ret;
}

// Extracts a C string from a V8 Utf8Value.
const char *ToCString(const v8::String::Utf8Value &value) {
  return *value ? *value : "<string conversion failed>";
}

void initAegisubModule(v8::Local<v8::Object> exports) {
  printf("initAegisubModule\n");
  v8::Local<v8::Context> context =
      exports->GetCreationContext().ToLocalChecked();
  auto isolate = context->GetIsolate();

  /* aegisub.register_macro(script_name, script_description, cleantags_macro)
  aegisub.register_filter(script_name, script_description, 0,
  cleantags_filter)
  */

  TEST_IDL
  // TODO:
  //  const JavaScriptScript* script = // get external pointer to script object!
  /*
    const auto scriptProperties = OrConvertable("scriptProperties");
    scriptProperties.add({StringConvertable("")})
    scriptProperties.add({ObjectConvertable()})


   */

  exports
      ->Set(context, TO_V8("set_script_properties"),
            v8::Function::New(
                context,
                [](const v8::FunctionCallbackInfo<v8::Value> &args) {
                  const auto isolate = args.GetIsolate();

                  // const auto scriptProperties = StringConvertable("test");

                  // TODO get an ArgumentPArser

                  const auto val = args[0];

                  //   const auto result = scriptProperties.from_v8(isolate,
                  //   val);

                  /*   if (!result.has_value()) {
                    } */

                  // TODO
                  // script.name = ...
                })
                .ToLocalChecked())
      .Check();
}

void addGlobalProperties(v8::Isolate *isolate, v8::Local<v8::Context> &context,
                         const std::filesystem::path &path,
                         const std::uint32_t timeout, const bool isModule) {

  // https://nodejs.org/docs/latest-v18.x/api/esm.html#no-__filename-or-__dirname
  if (!isModule) {
    context->Global()
        ->Set(context, TO_V8("__filename"), TO_V8(path.string()))
        .Check();

    context->Global()
        ->Set(context, TO_V8("__dirname"), TO_V8(path.parent_path().string()))
        .Check();
  }

  v8::Local<v8::Object> internalProperties = v8::Object::New(isolate);
  internalProperties
      ->Set(context, TO_V8("fileContent"), TO_V8(read_content(path)))
      .Check();

  internalProperties->Set(context, TO_V8("timeout"), TO_V8(timeout)).Check();

  internalProperties->Set(context, TO_V8("isModule"), TO_V8(isModule)).Check();

  internalProperties->Set(context, TO_V8("filename"), TO_V8(path.string()))
      .Check();

  v8::Local<v8::Array> linkedBindingsArray =
      VECTOR_TO_V8(std::vector<const char *>{"aegisub"});

  internalProperties->Set(context, TO_V8("linkedBindings"), linkedBindingsArray)
      .Check();

  context->Global()
      ->Set(context, TO_V8("__internal__properties__"), internalProperties)
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
                    const std::filesystem::path &path, const bool isModule,
                    const std::uint32_t timeout) {
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

    addGlobalProperties(isolate, context, path, timeout, isModule);

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
        "Object.keys(publicRequire).forEach(key=>{modifiedRequire[key] = "
        "publicRequire[key];});\n"
        "globalThis.require = modifiedRequire;\n"
        "globalThis.module = {children: [], exports: {}, filename: filename, "
        "id: filename, isPreloading: false, loaded: true, path: "
        "globalThis.__dirname, paths: "
        "globalThis.require.resolve.paths('__external__'), require: "
        "globalThis.require};\n"
        "globalThis.exports = globalThis.module.exports;\n" // TODO: this
                                                            // doesn't work as
                                                            // intended yet!
        "}\n"
        "const script = "
        "new _vm.Script(fileContent,{filename: filename});\n"
        // runInThisContext has only access to globals (globalThis), but
        // not locals!
        "let exitCode = 0;\n"
        "globalThis.process.exit = (num)=>{exitCode=num;};\n"
        "new _vm.Script(fileContent,{filename: filename});\n"
        "const last_statement_result = "
        "script.runInThisContext({displayErrors:true, "
        "timeout: timeout});\n" // after running for 'timeout' milli-seconds the
                                // script gets killed!
        "const result = {exitCode, result: globalThis.module.exports};\n"
        "console.log(result);\n"
        "return result;\n");

    // There has been a JS exception.
    if (loadenv_ret.IsEmpty()) {
      // TODO get JS exception and print
      std::cerr << "Error occurred in executing js\n";
      return 1;
    }

    const auto result = loadenv_ret.ToLocalChecked();

    if (!result->IsObject()) {
      std::cerr << "Incorrect return value of environment!\n";
      return 2;
    }
    v8::Local<v8::Object> object = result->ToObject(context).ToLocalChecked();

    const v8::Local<v8::Value> exitCode =
        object->Get(context, TO_V8("exitCode")).ToLocalChecked();

    const std::uint32_t exitCodeInt =
        exitCode->ToUint32(context).ToLocalChecked()->Value();

    std::cout << "exitCode: " << exitCodeInt << "\n";

    // TODO: extract exports, and assert correct exports (since it may be
    // arbitrary js)

    // get name etc from loadenv_ret and call callback
    // e.g. export default {name:"a", description: "test",author:
    // "me",version: "0.0.1"}

    exit_code = node::SpinEventLoop(env).FromMaybe(1);

    node::Stop(env);
  }

  return exit_code;
}

} // namespace JS
} // namespace Automation4
