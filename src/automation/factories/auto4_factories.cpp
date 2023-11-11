

#include <memory>
#include <vector>

#include "auto4_base.h"
#include "auto4_factories.hpp"
#include "auto4_lua_factory.h"

#ifdef ENABLE_JS_AUTOMATION
#include "auto4_js_factory.h"
#endif

#include <libaegisub/make_unique.h>

namespace Automation4 {

std::vector<std::unique_ptr<ScriptFactory>> Factories::createAll() {
  auto all = std::vector<std::unique_ptr<ScriptFactory>>{};
  all.push_back(agi::make_unique<Automation4::LuaScriptFactory>());
#ifdef ENABLE_JS_AUTOMATION
  all.push_back(agi::make_unique<Automation4::JavaScriptFactory>());
#endif

  return all;
}

} // namespace Automation4
