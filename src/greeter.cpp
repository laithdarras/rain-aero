#include "greeter.h"

#include <fmt/format.h>

namespace quickstart
{

std::string greet(std::string_view name)
{
  return fmt::format("Hello, {}!", name);
}

}  // namespace quickstart
