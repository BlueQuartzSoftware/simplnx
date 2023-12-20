#include "simplnx/DataStructure/LinkedPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"

#include <utility>

using namespace nx::core;

LinkedPath::LinkedPath() = default;

LinkedPath::LinkedPath(const DataStructure* dataStructure, const std::vector<DataObject::IdType>& idPath)
: m_DataStructure(dataStructure)
, m_IdPath(idPath)
{
}

LinkedPath::LinkedPath(const LinkedPath& rhs) = default;

LinkedPath::LinkedPath(LinkedPath&& rhs) noexcept
: m_DataStructure(std::exchange(rhs.m_DataStructure, nullptr))
, m_IdPath(std::move(rhs.m_IdPath))
{
}

LinkedPath& LinkedPath::operator=(const LinkedPath& rhs) = default;

LinkedPath& LinkedPath::operator=(LinkedPath&& rhs) noexcept
{
  m_DataStructure = std::exchange(rhs.m_DataStructure, nullptr);
  m_IdPath = std::move(rhs.m_IdPath);
  return *this;
}

LinkedPath::~LinkedPath() = default;

bool LinkedPath::isValid() const
{
  if(!m_DataStructure)
  {
    return false;
  }
  if(getLength() == 0)
  {
    return false;
  }
  for(usize i = 0; i < getLength(); i++)
  {
    if(getDataAt(i) == nullptr)
    {
      return false;
    }
  }
  return true;
}

const DataStructure* LinkedPath::getDataStructure() const
{
  return m_DataStructure;
}

DataPath LinkedPath::toDataPath() const
{
  std::vector<std::string> names(m_IdPath.size());
  for(usize i = 0; i < m_IdPath.size(); i++)
  {
    names[i] = getNameAt(i);
  }
  return DataPath(names);
}

usize LinkedPath::getLength() const
{
  return m_IdPath.size();
}

DataObject::IdType LinkedPath::operator[](usize index) const
{
  return getIdAt(index);
}

DataObject::IdType LinkedPath::getId() const
{
  return getIdAt(getLength() - 1);
}

DataObject::IdType LinkedPath::getIdAt(usize index) const
{
  return m_IdPath[index];
}

const DataObject* LinkedPath::getData() const
{
  return getDataAt(m_IdPath.size() - 1);
}

const DataObject* LinkedPath::getDataAt(usize index) const
{
  return getDataStructure()->getData(m_IdPath[index]);
}

std::string LinkedPath::getName() const
{
  return getData()->getName();
}

std::string LinkedPath::getNameAt(usize index) const
{
  const DataObject* data = getDataAt(index);
  if(data == nullptr)
  {
    return "[ missing ]";
  }
  return data->getName();
}

std::string LinkedPath::toString(const std::string& div) const
{
  return toDataPath().toString(div);
}

bool LinkedPath::operator==(const LinkedPath& rhs) const
{
  if(getLength() != rhs.getLength())
  {
    return false;
  }

  return (m_DataStructure == rhs.m_DataStructure) && (m_IdPath == rhs.m_IdPath);
}

bool LinkedPath::operator==(const DataPath& rhs) const
{
  if(getLength() != rhs.getLength())
  {
    return false;
  }

  for(usize i = 0; i < m_IdPath.size(); i++)
  {
    if(getNameAt(i) != rhs[i])
    {
      return false;
    }
  }
  return true;
}

bool LinkedPath::operator!=(const LinkedPath& rhs) const
{
  if(getLength() != rhs.getLength())
  {
    return true;
  }
  return (m_DataStructure != rhs.m_DataStructure) || (m_IdPath != rhs.m_IdPath);
}

bool LinkedPath::operator!=(const DataPath& rhs) const
{
  if(getLength() != rhs.getLength())
  {
    return true;
  }

  for(usize i = 0; i < m_IdPath.size(); i++)
  {
    if(getNameAt(i) != rhs[i])
    {
      return true;
    }
  }
  return false;
}
