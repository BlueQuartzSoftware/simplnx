#include "DataObject.hpp"

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5ObjectWriter.hpp"

#include <algorithm>
#include <stdexcept>

using namespace complex;

namespace complex
{
bool DataObject::IsValidName(std::string_view name)
{
  return name.find('/') == std::string_view::npos;
}

DataObject::DataObject(DataStructure& ds, std::string name)
: DataObject(ds, std::move(name), ds.generateId())
{
}

DataObject::DataObject(DataStructure& ds, std::string name, IdType importId)
: m_DataStructure(&ds)
, m_Id(importId)
, m_Name(std::move(name))
{
  if(!IsValidName(m_Name))
  {
    throw std::invalid_argument("DataObject names cannot contain \"/\"");
  }
}

DataObject::DataObject(const DataObject& rhs) = default;

DataObject::DataObject(DataObject&& rhs) = default;

DataObject& DataObject::operator=(const DataObject& rhs) = default;

DataObject& DataObject::operator=(DataObject&& rhs) noexcept = default;

DataObject::~DataObject() noexcept
{
  if(m_DataStructure == nullptr)
  {
    return;
  }
  if(m_DataStructure->m_IsValid)
  {
    m_DataStructure->dataDeleted(getId(), getName());
  }
}

bool DataObject::AttemptToAddObject(DataStructure& ds, const std::shared_ptr<DataObject>& data, const std::optional<IdType>& parentId)
{
  return ds.finishAddingObject(data, parentId);
}

DataObject::IdType DataObject::getId() const
{
  return m_Id;
}

DataStructure* DataObject::getDataStructure()
{
  return m_DataStructure;
}

const DataStructure* DataObject::getDataStructure() const
{
  return m_DataStructure;
}

void DataObject::setDataStructure(DataStructure* ds)
{
  m_DataStructure = ds;
}

std::string DataObject::getName() const
{
  return m_Name;
}

bool DataObject::canRename(const std::string& name) const
{
  if(!IsValidName(name))
  {
    return false;
  }

  auto dataStruct = getDataStructure();
  return !std::any_of(m_ParentList.cbegin(), m_ParentList.cend(), [dataStruct, name](IdType parentId) { return dataStruct->getDataAs<BaseGroup>(parentId)->contains(name); });
}

bool DataObject::rename(const std::string& name)
{
  if(!canRename(name))
  {
    return false;
  }

  m_Name = name;
  return true;
}

DataObject::ParentCollectionType DataObject::getParentIds() const
{
  return m_ParentList;
}

Metadata& DataObject::getMetadata()
{
  return m_Metadata;
}

const Metadata& DataObject::getMetadata() const
{
  return m_Metadata;
}

void DataObject::addParent(BaseGroup* parent)
{
  m_ParentList.push_back(parent->getId());
}

void DataObject::removeParent(BaseGroup* parent)
{
  m_ParentList.remove(parent->getId());
}

void DataObject::replaceParent(BaseGroup* oldParent, BaseGroup* newParent)
{
  std::replace(m_ParentList.begin(), m_ParentList.end(), oldParent->getId(), newParent->getId());
}

std::vector<DataPath> DataObject::getDataPaths() const
{
  if(m_ParentList.empty())
  {
    return {DataPath({getName()})};
  }

  std::vector<DataPath> paths;
  for(const auto& parentId : m_ParentList)
  {
    auto parent = getDataStructure()->getData(parentId);
    if(parent == nullptr)
    {
      continue;
    }

    auto parentPaths = parent->getDataPaths();
    for(auto& dataPath : parentPaths)
    {
      paths.push_back(dataPath.createChildPath(m_Name));
    }
  }
  return paths;
}

H5::ErrorType DataObject::writeH5ObjectAttributes(H5::DataStructureWriter& dataStructureWriter, H5::ObjectWriter& objectWriter) const
{
  // Add to DataStructureWriter for use in linking
  dataStructureWriter.addH5Writer(objectWriter, getId());

  auto typeAttributeWriter = objectWriter.createAttribute(complex::Constants::k_ObjectTypeTag);
  auto error = typeAttributeWriter.writeString(getTypeName());
  if(error < 0)
  {
    return error;
  }

  auto idAttributeWriter = objectWriter.createAttribute(complex::Constants::k_ObjectIdTag);
  error = idAttributeWriter.writeValue(getId());

  return error;
}
} // namespace complex
