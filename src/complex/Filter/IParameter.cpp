#include "IParameter.hpp"

namespace complex
{
std::any IParameter::construct(const Arguments& args) const
{
  return args.at(name());
}
} // namespace complex
