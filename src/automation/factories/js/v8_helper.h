

#include <memory>
#include <string>
#include <tl/expected.hpp>
#include <vector>

#include "node.h"
#include "uv.h"

struct JSState {
  v8::Isolate *isolate;
  node::Environment *env;
};

// TODO: use something like cpu or thread count!
#define THREAD_POOL_SIZE 4

tl::expected<JSState *, std::string> js_new_state(std::string const &filename);

std::string js_stop_state(JSState *state);
