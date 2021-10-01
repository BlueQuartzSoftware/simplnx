#include "Parameters.hpp"

namespace
{
void cloneParams(const std::map<std::string, std::unique_ptr<complex::IParameter>, std::less<>>& inputParams, std::map<std::string, std::unique_ptr<complex::IParameter>, std::less<>>& outputParams)
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

bool Parameters::contains(std::string_view name) const
{
  return m_Params.count(name) > 0;
}

usize Parameters::size() const
{
  return m_Params.size();
}

bool Parameters::empty() const
{
  return m_Params.empty();
}

void Parameters::insert(IParameter::UniquePointer parameter)
{
  std::string name = parameter->name();
  m_Params.insert({std::move(name), std::move(parameter)});
}

std::vector<std::string> Parameters::getKeys() const
{
  std::vector<std::string> acceptedKeys;
  for(const auto& param : m_Params)
  {
    acceptedKeys.push_back(param.first);
  }
  return acceptedKeys;
}

} // namespace complex
