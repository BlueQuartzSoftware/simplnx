#pragma once

#include <string>
#include <vector>

#include "complex/Common/Types.hpp"

namespace complex
{
struct Error
{
  i32 code = 0;
  std::string message;
};

struct Warning
{
  i32 code = 0;
  std::string message;
};

struct Result
{
  std::vector<Error> errors;
  std::vector<Warning> warnings;
};
} // namespace complex
