#include "IParameter.hpp"

namespace nx::core
{
std::any IParameter::construct(const Arguments& args) const
{
  return args.at(name());
}
} // namespace nx::core
