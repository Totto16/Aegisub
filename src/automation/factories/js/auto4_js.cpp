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
#include <libaegisub/log.h>
#include <libaegisub/make_unique.h>
#include <libaegisub/path.h>

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

// TODO: make this options!
#define DEFAULT_JS_TYPE JSType::CommonJS
#define SCRIPT_EXECUTION_TIMEOUT_MS (60 * 1000) // in milliseconds!

using namespace Automation4::JS;

namespace Automation4 {
namespace JS {
JSType get_type(agi::fs::path const &filename) {
  if (agi::fs::HasExtension(filename, "js")) {
    return DEFAULT_JS_TYPE;
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
  bool loaded;

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
  bool GetLoadedState() const override { return this->loaded; }

  std::vector<cmd::Command *> GetMacros() const override { return macros; }
  std::vector<ExportFilter *> GetFilters() const override;
};

JavaScriptScript::JavaScriptScript(agi::fs::path const &filename)
    : Script(filename), type{get_type(filename)} {
  Create();
}
void JavaScriptScript::Create() {
  Destroy();

  this->name = GetPrettyFilename().string();

  LOG_D("JavaScriptScript/initialize") << "Initialize new state";

  this->loaded = false;

  execute_js_file(this->GetFilename().string(), this->type,
                  SCRIPT_EXECUTION_TIMEOUT_MS,
                  [this](std::string name, std::string description,
                         std::string author, std::string version) {
                    this->loaded = true;
                    if (name.empty()) {
                      this->name = GetPrettyFilename().string();
                    } else {
                      this->name = name;
                    }
                    this->description = description;
                    this->author = author;
                    this->version = version;
                  });
}

void JavaScriptScript::Destroy() {
  // Assume the script object is clean if it's not loaded
  if (!this->loaded) {
    return;
  }

  this->loaded = false;
}

std::vector<ExportFilter *> JavaScriptScript::GetFilters() const {
  std::vector<ExportFilter *> ret;
  /* 		ret.reserve(filters.size());
                  for (auto& filter : filters) ret.push_back(filter.get()); */
  return ret;
}

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
} // namespace JS
} // namespace Automation4
