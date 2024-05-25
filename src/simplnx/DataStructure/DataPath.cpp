#include "DataPath.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <stdexcept>

using namespace nx::core;

namespace nx::core
{
DataPath::DataPath() = default;

DataPath::DataPath(std::vector<std::string> path)
: m_Path(std::move(path))
{
  for(const auto& item : m_Path)
  {
    if(!DataObject::IsValidName(item) && !item.empty())
    {
      throw std::invalid_argument(fmt::format("DataPath: Invalid DataObject name - [{}]. One of the DataObject names contains the '/' character.", fmt::join(m_Path, ",")));
    }
  }
}

DataPath::DataPath(const DataPath& rhs) = default;

DataPath::DataPath(DataPath&& rhs) noexcept = default;

DataPath& DataPath::operator=(const DataPath& rhs) = default;

DataPath& DataPath::operator=(DataPath&& rhs) noexcept = default;

DataPath::~DataPath() noexcept = default;

std::optional<DataPath> DataPath::FromString(std::string_view inputPath, char delimiter)
{
  if(inputPath.empty())
  {
    return DataPath{};
  }
  if(inputPath.size() == 1 && inputPath[0] == delimiter)
  {
    return DataPath{};
  }
  auto parts = StringUtilities::split(inputPath, delimiter);
  if(parts.empty())
  {
    return DataPath{};
  }
  try
  {
    return DataPath(std::move(parts));
  } catch(const std::invalid_argument& exception)
  {
    return {};
  }
}

usize DataPath::getLength() const
{
  return m_Path.size();
}

bool DataPath::empty() const
{
  return m_Path.empty();
}

std::string DataPath::getTargetName() const
{
  if(empty())
  {
    return "";
  }
  return m_Path.back();
}

std::vector<std::string> DataPath::getPathVector() const
{
  return m_Path;
}

DataPath DataPath::getParent() const
{
  if(getLength() <= 1)
  {
    return {};
  }

  std::vector<std::string> parentPath(m_Path.cbegin(), m_Path.cend() - 1);
  return {(std::move(parentPath))};
}

DataPath DataPath::createChildPath(std::string name) const
{
  std::vector<std::string> path = m_Path;
  path.push_back(std::move(name));
  return {(std::move(path))};
}

DataPath DataPath::replaceName(const std::string& newName) const
{
  if(m_Path.empty())
  {
    return DataPath({newName});
  }
  std::vector<std::string> newPath = m_Path;
  newPath.back() = newName;
  return {(std::move(newPath))};
}

DataPath DataPath::replace(std::string_view symbol, std::string_view targetName) const
{
  std::vector<std::string> newPath = m_Path;
  std::replace(newPath.begin(), newPath.end(), symbol, targetName);
  return {(std::move(newPath))};
}

bool DataPath::attemptRename(const DataPath& oldPath, const DataPath& newPath)
{
  if(oldPath.getLength() > getLength())
  {
    return false;
  }

  for(usize i = 0; i < oldPath.getLength(); i++)
  {
    if(oldPath.m_Path[i] != m_Path[i])
    {
      return false;
    }
  }

  for(usize i = 0; i < oldPath.getLength(); i++)
  {
    m_Path[i] = newPath.m_Path[i];
  }
  return true;
}

bool DataPath::operator==(const DataPath& rhs) const
{
  return m_Path == rhs.m_Path;
}

bool DataPath::operator!=(const DataPath& rhs) const
{
  return !(*this == rhs);
}

bool DataPath::operator<(const DataPath& rhs) const
{
  return toString() < rhs.toString();
}

bool DataPath::operator>(const DataPath& rhs) const
{
  return toString() > rhs.toString();
}

const std::string& DataPath::operator[](usize index) const
{
  return m_Path.at(index);
}

std::string DataPath::toString(std::string_view div) const
{
  return fmt::format("{}", fmt::join(m_Path, div));
}
} // namespace nx::core
