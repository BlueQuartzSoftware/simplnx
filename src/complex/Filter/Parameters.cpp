#include "Parameters.hpp"

namespace complex
{
void Parameters::insert(std::unique_ptr<IParameter> parameter)
{
  std::string name = parameter->name();
  m_Params.insert({std::move(name), std::move(parameter)});
}
} // namespace complex
