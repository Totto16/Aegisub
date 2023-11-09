

#include <memory>
#include <tl/expected.hpp>
#include <vector>

#include "node.h"
#include "uv.h"
#include "v8_helper.h"

tl::expected<JSState *, std::string> js_new_state(std::string const &filename) {

  std::vector<std::string> args = {filename};
  std::unique_ptr<node::InitializationResult> result =
      node::InitializeOncePerProcess( // TODO
          args,
          {node::ProcessInitializationFlags::kNoInitializeV8,
           node::ProcessInitializationFlags::kNoInitializeNodeV8Platform});

  for (const std::string &error : result->errors())
    fprintf(stderr, "%s: %s\n", args[0].c_str(), error.c_str());
  if (result->early_return() != 0) {
    return tl::make_unexpected("A startup occorred, ecxit code: " +
                               result->exit_code());
  }

  std::unique_ptr<node::MultiIsolatePlatform> platform =
      node::MultiIsolatePlatform::Create(THREAD_POOL_SIZE);
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();

  // Setup up a libuv event loop, v8::Isolate, and Node.js Environment.
  std::vector<std::string> errors;
  std::unique_ptr<node::CommonEnvironmentSetup> setup =
      node::CommonEnvironmentSetup::Create(platform.get(), &errors,
                                           result->args(), result->exec_args());
  if (!setup) {
    for (const std::string &err : errors) {
      fprintf(stderr, "%s: %s\n", args[0].c_str(), err.c_str());
    }
    return tl::make_unexpected("An error occurred");
  }

  v8::Isolate *isolate = setup->isolate();
  node::Environment *env = setup->env();

  return new JSState{isolate, env};
}

std::string js_stop_state(JSState *state) {

  node::Stop(state->env);

  v8::V8::Dispose();
  v8::V8::DisposePlatform();

  node::TearDownOncePerProcess();

  return "";
}
/**

{
    Locker locker(isolate);
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    // The v8::Context needs to be entered when node::CreateEnvironment() and
    // node::LoadEnvironment() are being called.
    Context::Scope context_scope(setup->context());

    // Set up the Node.js instance for execution, and run code inside of it.
    // There is also a variant that takes a callback and provides it with
    // the `require` and `process` objects, so that it can manually compile
    // and run scripts as needed.
    // The `require` function inside this script does *not* access the file
    // system, and can only load built-in Node.js modules.
    // `module.createRequire()` is being used to create one that is able to
    // load files from the disk, and uses the standard CommonJS file loader
    // instead of the internal-only `require` function.
    MaybeLocal<Value> loadenv_ret = node::LoadEnvironment(
        env, "const publicRequire ="
             "  require('node:module').createRequire(process.cwd() + '/');"
             "globalThis.require = publicRequire;"
             "require('node:vm').runInThisContext(process.argv[1]);");

    if (loadenv_ret.IsEmpty()) // There has been a JS exception.
      return 1;

    exit_code = node::SpinEventLoop(env).FromMaybe(1);

    // node::Stop() can be used to explicitly stop the event loop and keep
    // further JavaScript from running. It can be called from any thread,
    // and will act like worker.terminate() if called from another thread.
    node::Stop(env);
  }


 */
