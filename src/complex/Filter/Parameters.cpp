#include "Parameters.hpp"

namespace complex
{
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
  if(parameter == nullptr)
  {
    throw std::runtime_error("Parameters does not accept null IParameter");
  }

  std::string name = parameter->name();

  if(contains(name))
  {
    throw std::runtime_error("Parameters does not accept duplicate names");
  }

  m_Params.insert({name, std::move(parameter)});

  m_LayoutVector.push_back(ParameterKey{name});
}

void Parameters::insert(Separator name)
{
  m_LayoutVector.push_back(std::move(name));
}

std::vector<std::string> Parameters::getKeys() const
{
  std::vector<std::string> keys;
  for(const auto& item : m_LayoutVector)
  {
    if(const auto* parameterKey = std::get_if<ParameterKey>(&item); parameterKey != nullptr)
    {
      keys.push_back(parameterKey->key);
    }
  }
  return keys;
}

const Parameters::LayoutVector& Parameters::getLayout() const
{
  return m_LayoutVector;
}

} // namespace complex
