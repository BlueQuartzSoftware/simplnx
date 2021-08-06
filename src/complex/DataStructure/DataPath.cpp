#include "DataPath.hpp"

#include <algorithm>
#include <stdexcept>

#include "complex/Common/Types.hpp"

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

DataPath::DataPath(const std::vector<std::string>& path)
: m_Path(path)
{
}

DataPath::DataPath(const DataPath& other)
: m_Path(other.m_Path)
{
}

DataPath::DataPath(DataPath&& other) noexcept
: m_Path(std::move(other.m_Path))
{
}

DataPath::~DataPath() = default;

std::optional<DataPath> DataPath::FromString(std::string_view string, char delimiter)
{
  auto parts = split<std::string>(string, delimiter);
  if(parts.empty())
  {
    return {};
  }
  return DataPath(parts);
}

size_t DataPath::getLength() const
{
  return m_Path.size();
}

std::vector<std::string> DataPath::getPathVector() const
{
  return m_Path;
}

DataPath DataPath::getParent() const
{
  std::vector<std::string> parentPath = m_Path;
  parentPath.pop_back();
  return DataPath(parentPath);
}

DataPath DataPath::createChildPath(const std::string& name) const
{
  std::vector<std::string> path(m_Path);
  path.push_back(name);
  return DataPath(path);
}

DataPath DataPath::replace(const std::string& symbol, const std::string& targetName)
{
  std::vector<std::string> newPath = m_Path;
  std::replace(newPath.begin(), newPath.end(), symbol, targetName);
  return DataPath(newPath);
}

bool DataPath::operator==(const DataPath& rhs) const
{
  if(rhs.m_Path.size() != m_Path.size())
  {
    return false;
  }

  for(size_t i = 0; i < m_Path.size(); i++)
  {
    if(rhs.m_Path[i] != m_Path[i])
    {
      return false;
    }
  }
  return true;
}

bool DataPath::operator!=(const DataPath& rhs) const
{
  if(rhs.m_Path.size() != m_Path.size())
  {
    return true;
  }

  for(size_t i = 0; i < m_Path.size(); i++)
  {
    if(rhs.m_Path[i] != m_Path[i])
    {
      return true;
    }
  }
  return false;
}

const std::string& DataPath::operator[](size_t index) const
{
  if(index > m_Path.size())
  {
    throw std::runtime_error("");
  }
  return m_Path[index];
}

std::string DataPath::toString(const std::string& div) const
{
  std::string output;
  for(size_t i = 0; i < getLength(); i++)
  {
    if(i != 0)
    {
      output += div;
    }
    output += m_Path[i];
  }
  return output;
}
