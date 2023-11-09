// Copyright (c) 2006, 2007, Niels Martin Hansen
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Aegisub Project http://www.aegisub.org/

/// @file auto4_lua.cpp
/// @brief Lua 5.1-based scripting engine
/// @ingroup scripting
///

// TODO comments above

#include "auto4_js.h"
#include "v8_helper.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_info.h"
#include "ass_style.h"
#include "async_video_provider.h"
#include "audio_controller.h"
#include "audio_timing.h"
#include "auto4_js_factory.h"
#include "command/command.h"
#include "compat.h"
#include "frame_main.h"
#include "include/aegisub/context.h"
#include "options.h"
#include "project.h"
#include "selection_controller.h"
#include "subs_controller.h"
#include "utils.h"
#include "video_controller.h"

#include "libaegisub/fs.h"
#include <libaegisub/dispatch.h>
#include <libaegisub/format.h>
#include <libaegisub/make_unique.h>
#include <libaegisub/path.h>
#include <libaegisub/log.h>

#include <algorithm>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/scope_exit.hpp>
#include <cassert>
#include <mutex>
#include <stdexcept>
#include <wx/clipbrd.h>
#include <wx/log.h>
#include <wx/msgdlg.h>

using namespace Automation4;
namespace Automation4 {

JSType get_type(agi::fs::path const &filename) {
  if (agi::fs::HasExtension(filename, "js")) {
    return JSType::Auto;
  } else if (agi::fs::HasExtension(filename, "cjs")) {
    return JSType::CommonJS;

  } else if (agi::fs::HasExtension(filename, "mjs")) {
    return JSType::Module;
  } else {
    throw std::runtime_error("Extension not supported!");
  }
}

class JavaScriptScript final : public Script {
  std::string name;
  std::string description;
  std::string author;
  std::string version;

  std::vector<cmd::Command *> macros;
  std::vector<std::unique_ptr<ExportFilter>> filters;

  JSType type;
  JSState *state;

  /// load script and create internal structures etc.
  void Create();
  /// destroy internal structures, unreg features and delete environment
  void Destroy();

public:
  JavaScriptScript(agi::fs::path const &filename);
  ~JavaScriptScript() { Destroy(); }

  // void RegisterCommand(JSCommand *command);
  // void UnregisterCommand(JSCommand *command);
  // void RegisterFilter(LuaExportFilter *filter);

  // static JavaScriptScript *GetScriptObject(lua_State *L);

  // Script implementation
  void Reload() override { Create(); }

  std::string GetName() const override { return name; }
  std::string GetDescription() const override { return description; }
  std::string GetAuthor() const override { return author; }
  std::string GetVersion() const override { return version; }
  bool GetLoadedState() const override { return this->state != nullptr; }

  std::vector<cmd::Command *> GetMacros() const override { return macros; }
  std::vector<ExportFilter *> GetFilters() const override;
};

JavaScriptScript::JavaScriptScript(agi::fs::path const &filename)
    : Script(filename), type{get_type(filename)} {
  Create();
}
void JavaScriptScript::Create() {
  Destroy();

  name = GetPrettyFilename().string();


 LOG_D("JavaScriptScript/initialize")
        << "Initialize new state";

  // create new nodejs environment
  auto state = js_new_state(this->GetFilename().string());

  if (not state.has_value()) {
    description = "Could not initialize JS state: " + state.error();
    return;
  }

  this->state = state.value();

  bool loaded = false;
  BOOST_SCOPE_EXIT_ALL(&) {
    if (!loaded)
      Destroy();
  };

  // register standard libs
  // preload_modules(this.state);

  // dofile and loadfile are replaced with include
  /*  lua_pushnil(L);
   lua_setglobal(L, "dofile");
   lua_pushnil(L);
   lua_setglobal(L, "loadfile");
   push_value(L, exception_wrapper<LuaInclude>);
   lua_setglobal(L, "include"); */

  // Replace the default lua module loader with our unicode compatible
  // one and set the module search path
  /*  if (!Install(L, include_path)) {
     description = get_string_or_default(L, 1);
     lua_pop(L, 1);
     return;
   }
   stackcheck.check_stack(0); */

  // prepare stuff in the registry

  // store the script's filename
  /*  push_value(L, GetFilename().stem());
   lua_setfield(L, LUA_REGISTRYINDEX, "filename");
   stackcheck.check_stack(0); */

  // reference to the script object
  /* push_value(L, this);
  lua_setfield(L, LUA_REGISTRYINDEX, "aegisub");
  stackcheck.check_stack(0); */

  // make "aegisub" table
  /*  lua_pushstring(L, "aegisub");
   lua_createtable(L, 0, 13);

   set_field<JSCommand::LuaRegister>(L, "register_macro");
   set_field<LuaExportFilter::LuaRegister>(L, "register_filter");
   set_field<lua_text_textents>(L, "text_extents");
   set_field<frame_from_ms>(L, "frame_from_ms");
   set_field<ms_from_frame>(L, "ms_from_frame");
   set_field<video_size>(L, "video_size");
   set_field<get_keyframes>(L, "keyframes");
   set_field<decode_path>(L, "decode_path");
   set_field<cancel_script>(L, "cancel");
   set_field(L, "lua_automation_version", 4);
   set_field<clipboard_init>(L, "__init_clipboard");
   set_field<get_file_name>(L, "file_name");
   set_field<get_translation>(L, "gettext");
   set_field<project_properties>(L, "project_properties");
   set_field<lua_get_audio_selection>(L, "get_audio_selection");
   set_field<lua_set_status_text>(L, "set_status_text"); */

  // store aegisub table to globals
  /*  lua_settable(L, LUA_GLOBALSINDEX);
   stackcheck.check_stack(0); */

  // load user script
  /*  if (!LoadFile(L, GetFilename())) {
     description = get_string_or_default(L, 1);
     lua_pop(L, 1);
     return;
   }
   stackcheck.check_stack(1); */

  // Insert our error handler under the user's script
  /*  lua_pushcclosure(L, add_stack_trace, 0);
   lua_insert(L, -2); */

  // and execute it
  // this is where features are registered
  /*  if (lua_pcall(L, 0, 0, -2)) { */
  // error occurred, assumed to be on top of Lua stack
  /*  description =
       agi::format("Error initialising Lua script \"%s\":\n\n%s",
                   GetPrettyFilename().string(), get_string_or_default(L, -1));
   lua_pop(L, 2); // error + error handler
   return;
 }
 lua_pop(L, 1); // error handler
 stackcheck.check_stack(0); */

  /*  lua_getglobal(L, "version");
   if (lua_isnumber(L, -1) && lua_tointeger(L, -1) == 3) {
     lua_pop(L, 1); // just to avoid tripping the stackcheck in debug
     description = "Attempted to load an Automation 3 script as an Automation 4
   " "Lua script. Automation 3 is no longer supported."; return;
   } */

  /*   name = get_global_string(L, "script_name");
    description = get_global_string(L, "script_description");
    author = get_global_string(L, "script_author");
    version = get_global_string(L, "script_version");

    if (name.empty())
      name = GetPrettyFilename().string();

    lua_pop(L, 1); */
  // if we got this far, the script should be ready
  loaded = true;
}

void JavaScriptScript::Destroy() {
  // Assume the script object is clean if there's no JS state
  if (!this->state)
    return;

  auto result = js_stop_state(this->state);

  if (not result.empty()) {
    std::cerr << "An error occurred while stopping js vm: " << result;
    return;
  }

  this->state = nullptr;
}

std::vector<ExportFilter *> JavaScriptScript::GetFilters() const {
  std::vector<ExportFilter *> ret;
  /* 		ret.reserve(filters.size());
                  for (auto& filter : filters) ret.push_back(filter.get()); */
  return ret;
}

} // namespace Automation4

namespace Automation4 {
JavaScriptFactory::JavaScriptFactory()
    : ScriptFactory("JavaScript", "*.js,*.cjs,*.mjs") {}

std::unique_ptr<Script>
JavaScriptFactory::Produce(agi::fs::path const &filename) const {
  if (agi::fs::HasExtension(filename, "js") ||
      agi::fs::HasExtension(filename, "cjs") ||
      agi::fs::HasExtension(filename, "mjs")) {
    LOG_D("JavaScriptFactory/load")
        << "Loading JS Filename: " << filename.string();
    return agi::make_unique<JavaScriptScript>(filename);
  }
  return nullptr;
}
} // namespace Automation4
