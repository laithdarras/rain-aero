#pragma once

#include <string>
#include <string_view>

namespace quickstart
{

  /// Return a greeting for `name`, e.g. `greet("world") -> "Hello, world!"`.
  std::string greet(std::string_view name);

}  // namespace quickstart
