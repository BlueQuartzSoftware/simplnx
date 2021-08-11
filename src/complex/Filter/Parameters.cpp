#include "Parameters.hpp"

namespace
{
void cloneParams(const std::map<std::string, std::unique_ptr<complex::IParameter>>& inputParams, std::map<std::string, std::unique_ptr<complex::IParameter>>& outputParams)
{
  for(const auto& [key, value] : inputParams)
  {
    outputParams.insert({key, value->clone()});
  }
}
} // namespace

namespace complex
{
Parameters::Parameters(const Parameters& rhs)
{
  cloneParams(rhs.m_Params, m_Params);
}

Parameters& Parameters::operator=(const Parameters& rhs)
{
  if(this == &rhs)
  {
    return *this;
  }
  m_Params.clear();
  cloneParams(rhs.m_Params, m_Params);
  return *this;
}

void Parameters::insert(std::unique_ptr<IParameter> parameter)
{
  std::string name = parameter->name();
  m_Params.insert({std::move(name), std::move(parameter)});
}
} // namespace complex
