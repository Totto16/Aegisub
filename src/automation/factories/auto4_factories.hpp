
#pragma once

#include <vector>
#include <memory>

#include "auto4_base.h"

namespace Automation4 {

class Factories {
public:
  static std::vector<std::unique_ptr<ScriptFactory>> createAll();
};

}; // namespace Automation4
