#include "DataPath.hpp"

#include <algorithm>
#include <stdexcept>

#include <fmt/format.h>

#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataObject.hpp"

using namespace complex;

namespace
{
template <class T = std::string_view>
std::vector<T> split(std::string_view string, char delimiter, bool ignoreEmpty = false)
{
  std::vector<T> parts;

  usize start = 0;
  usize end = start;

  if(ignoreEmpty)
  {
    for(usize i = 0; i < string.size(); i++)
    {
      if(string[i] == delimiter)
      {
        if(start == end)
        {
          start = i + 1;
          end = start;
          continue;
        }
        parts.push_back(T{string.substr(start, end - start)});
        start = i + 1;
        end = start;
        continue;
      }
      end++;
    }

    if(start != end)
    {
      parts.push_back(T{string.substr(start, end - start)});
    }
  }
  else
  {
    for(usize i = 0; i < string.size(); i++)
    {
      if(string[i] == delimiter)
      {
        parts.push_back(T{string.substr(start, end - start)});
        start = i + 1;
        end = start;
        continue;
      }
      end++;
    }

    parts.push_back(T{string.substr(start, end - start)});
  }

  return parts;
}
} // namespace

namespace complex
{
DataPath::DataPath() = default;

DataPath::DataPath(std::vector<std::string> path)
: m_Path(std::move(path))
{
  for(const auto& item : m_Path)
  {
    if(!DataObject::IsValidName(item))
    {
      throw std::invalid_argument("DataPath: Invalid DataObject name");
    }
  }
}

DataPath::DataPath(const DataPath& rhs) = default;

DataPath::DataPath(DataPath&& rhs) noexcept = default;

DataPath& DataPath::operator=(const DataPath& rhs) = default;

DataPath& DataPath::operator=(DataPath&& rhs) noexcept = default;

DataPath::~DataPath() noexcept = default;

std::optional<DataPath> DataPath::FromString(std::string_view string, char delimiter)
{
  auto parts = split<std::string>(string, delimiter);
  if(parts.empty())
  {
    return {};
  }
  return DataPath(std::move(parts));
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
  return DataPath(std::move(parentPath));
}

DataPath DataPath::createChildPath(std::string name) const
{
  std::vector<std::string> path = m_Path;
  path.push_back(std::move(name));
  return DataPath(std::move(path));
}

DataPath DataPath::replace(std::string_view symbol, std::string_view targetName) const
{
  std::vector<std::string> newPath = m_Path;
  std::replace(newPath.begin(), newPath.end(), symbol, targetName);
  return DataPath(std::move(newPath));
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
} // namespace complex
