#include "DataPath.h"

#include <algorithm>

using namespace SIMPL;

// Constructors/Destructors
//

DataPath::DataPath(const std::vector<std::string>& path)
: m_Path(path)
{
}

DataPath::DataPath(const std::vector<std::string>& path, const std::string& child)
: m_Path(path)
{
  m_Path.push_back(child);
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

//
// Methods
//

size_t DataPath::getLength() const
{
  return m_Path.size();
}

std::vector<std::string> DataPath::getPathVector() const
{
  return m_Path;
}
SIMPL::DataPath DataPath::getParent() const
{
  std::vector<std::string> parentPath = m_Path;
  parentPath.pop_back();
  return DataPath(parentPath);
}
SIMPL::DataPath DataPath::createChildPath(const std::string& name) const
{
  return DataPath(m_Path, name);
}
SIMPL::DataPath DataPath::replace(const std::string& symbol, const std::string& targetName)
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
    throw std::exception();
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
