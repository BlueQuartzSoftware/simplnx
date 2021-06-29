#include "complex/DataStructure/LinkedPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"

#include <exception>

using namespace complex;

LinkedPath::LinkedPath()
{
}

LinkedPath::LinkedPath(const DataStructure* ds, const std::vector<DataObject::IdType>& idPath)
: m_DataStructure(ds)
, m_IdPath(idPath)
{
}

LinkedPath::LinkedPath(const LinkedPath& rhs)
: m_DataStructure(rhs.m_DataStructure)
, m_IdPath(rhs.m_IdPath)
{
}

LinkedPath::LinkedPath(LinkedPath&& rhs) noexcept
: m_DataStructure(std::move(rhs.m_DataStructure))
, m_IdPath(std::move(rhs.m_IdPath))
{
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
  for(size_t i = 0; i < getLength(); i++)
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
  for(size_t i = 0; i < m_IdPath.size(); i++)
  {
    names[i] = getNameAt(i);
  }
  return DataPath(names);
}

size_t LinkedPath::getLength() const
{
  return m_IdPath.size();
}

DataObject::IdType LinkedPath::operator[](size_t index) const
{
  return getIdAt(index);
}

DataObject::IdType LinkedPath::getId() const
{
  return getIdAt(getLength() - 1);
}

DataObject::IdType LinkedPath::getIdAt(size_t index) const
{
  return m_IdPath[index];
}

const DataObject* LinkedPath::getData() const
{
  return getDataAt(m_IdPath.size() - 1);
}

const DataObject* LinkedPath::getDataAt(size_t index) const
{
  return getDataStructure()->getData(m_IdPath[index]);
}

std::string LinkedPath::getName() const
{
  return getData()->getName();
}

std::string LinkedPath::getNameAt(size_t index) const
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

  for(size_t i = 0; i < m_IdPath.size(); i++)
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

  for(size_t i = 0; i < m_IdPath.size(); i++)
  {
    if(getNameAt(i) != rhs[i])
    {
      return true;
    }
  }
  return false;
}
