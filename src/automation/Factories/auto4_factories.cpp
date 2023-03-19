

#include <vector>

#include "auto4_base.h"
#include "auto4_factories.hpp"
#include "auto4_lua_factory.h"

#include <libaegisub/make_unique.h>

namespace Automation4 {

std::vector<std::unique_ptr<ScriptFactory>> Factories::createAll() {
  auto all = std::vector<std::unique_ptr<ScriptFactory>>{};
  all.push_back(agi::make_unique<Automation4::LuaScriptFactory>());

  // TODO add here the js factory
  // all.push_back(agi::make_unique<Automation4::JavaScriptFactory>());

  return all;
}

} // namespace Automation4