#include "Parameters.hpp"

using namespace nx::core;

namespace
{
template <class ContainerT>
auto& MapAt(ContainerT& container, std::string_view key, const char* message)
{
  auto iter = container.find(key);
  if(iter == container.cend())
  {
    throw std::invalid_argument(fmt::format(message, key));
  }
  return iter->second;
}
} // namespace

namespace nx::core
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
    throw std::invalid_argument("Parameters::insert(parameter): Attempting to insert a null IParameter Object");
  }

  std::string name = parameter->name();

  if(contains(name))
  {
    throw std::invalid_argument(fmt::format("Parameters::insert(parameter): Attempting to insert a parameter with the same name as an already inserted IParameter. Conflicting name is '{}'", name));
  }

  m_Params.insert({name, std::move(parameter)});
  m_ParamGroups.insert({name, GroupList{}});

  m_LayoutVector.emplace_back(ParameterKey{name});
}

void Parameters::insert(Separator separator)
{
  insertSeparator(std::move(separator));
}

void Parameters::insertSeparator(Separator separator)
{
  m_LayoutVector.emplace_back(std::move(separator));
}

AnyParameter& Parameters::at(std::string_view key)
{
  return MapAt(m_Params, key, "Key '{}' does not exist in Parameters");
}

const AnyParameter& Parameters::at(std::string_view key) const
{
  return MapAt(m_Params, key, "Key '{}' does not exist in Parameters");
}

void Parameters::insertLinkableParameter(IParameter::UniquePointer parameter, IsActiveFunc func)
{
  std::string name = parameter->name();
  insert(std::move(parameter));
  m_Groups.insert({name, func});
}

void Parameters::linkParameters(std::string groupKey, std::string_view childKey, std::any associatedValue)
{
  if(!contains(childKey))
  {
    throw std::invalid_argument(fmt::format("Parameters::linkParameters(...): Child Key '{}' does not exist in Parameters instance", childKey));
  }

  if(!contains(groupKey))
  {
    throw std::invalid_argument(fmt::format("Parameters::linkParameters(...): Group Key '{}' does not exist in Parameters instance.", groupKey));
  }

  if(!containsGroup(groupKey))
  {
    throw std::invalid_argument(fmt::format("Parameters::linkParameters(...): Group '{}' does not exist in Parameters instance.", groupKey));
  }

  if(containsGroup(childKey))
  {
    throw std::invalid_argument(fmt::format("Parameters::linkParameters(...): Group '{}' cannot be a child of another group", childKey));
  }

  MapAt(m_ParamGroups, childKey, "Parameters::linkParameters(...): Key '{}' does not exist").push_back({std::move(groupKey), std::move(associatedValue)});
}

bool Parameters::isParameterActive(std::string_view key, const Arguments& args) const
{
  // If a parameter is not in a group, it is always active

  if(getNumberOfLinkedGroups(key) == 0)
  {
    return true;
  }

  // If a parameter is in at least one group, it's active if the stored value and argument value pass the stored IsActive function for all groups

  const auto& groupList = MapAt(m_ParamGroups, key, "Key '{}' does not have group in Parameters");

  bool isActive = false;

  for(const auto& [groupKey, associatedValue] : groupList)
  {
    const std::any& value = args.at(groupKey);
    IsActiveFunc func = MapAt(m_Groups, groupKey, "Group key '{}' does not exist in Parameters");

    const IParameter* groupParameter = at(groupKey).get();

    isActive |= func(*groupParameter, value, associatedValue);
  }

  return isActive;
}

bool Parameters::containsGroup(std::string_view key) const
{
  return m_Groups.count(key) > 0;
}

std::vector<std::string> Parameters::getKeysInGroup(std::string_view groupKey) const
{
  std::vector<std::string> keys;
  for(const auto& [paramKey, groupList] : m_ParamGroups)
  {
    for(const auto& pair : groupList)
    {
      if(pair.first == groupKey)
      {
        keys.push_back(paramKey);
      }
    }
  }
  return keys;
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

std::vector<std::string> Parameters::getGroupKeys() const
{
  std::vector<std::string> keys;
  for(const auto& pair : m_Groups)
  {
    keys.push_back(pair.first);
  }
  return keys;
}

const Parameters::LayoutVector& Parameters::getLayout() const
{
  return m_LayoutVector;
}

usize Parameters::getNumberOfLinkedGroups(std::string_view key) const
{
  return MapAt(m_ParamGroups, key, "Parameters::getNumberOfLinkedGroups(...): Key '{}' does not exist").size();
}

const Parameters::GroupList& Parameters::getLinkedGroups(std::string_view key) const
{
  return MapAt(m_ParamGroups, key, "Parameters::getLinkedGroups(...): Key '{}' does not exist");
}
} // namespace nx::core
