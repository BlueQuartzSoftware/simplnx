#include "DataObject.hpp"

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <algorithm>
#include <stdexcept>

using namespace nx::core;

namespace nx::core
{
bool DataObject::IsValidName(std::string_view name)
{
  return !name.empty() && name.find('/') == std::string_view::npos;
}

DataObject::DataObject(DataStructure& dataStructure, std::string name)
: DataObject(dataStructure, std::move(name), dataStructure.generateId())
{
}

DataObject::DataObject(DataStructure& dataStructure, std::string name, IdType importId)
: m_DataStructure(&dataStructure)
, m_Id(importId)
, m_Name(std::move(name))
{
  if(!IsValidName(m_Name))
  {
    throw std::invalid_argument("DataObject names cannot contain \"/\"");
  }
}

DataObject::DataObject(const DataObject& rhs)
: m_DataStructure(rhs.m_DataStructure)
, m_ParentList(rhs.m_ParentList)
, m_Id(rhs.m_Id)
, m_Name(rhs.m_Name)
, m_Metadata(rhs.m_Metadata)
{
}

DataObject::DataObject(DataObject&& rhs)
: m_DataStructure(rhs.m_DataStructure)
, m_ParentList(std::move(rhs.m_ParentList))
, m_Id(rhs.m_Id)
, m_Name(std::move(rhs.m_Name))
, m_Metadata(std::move(rhs.m_Metadata))
{
}

DataObject& DataObject::operator=(const DataObject& rhs)
{
  if(this == &rhs)
  {
    return *this;
  }
  m_DataStructure = rhs.m_DataStructure;
  m_ParentList = rhs.m_ParentList;
  m_Id = rhs.m_Id;
  m_Name = rhs.m_Name;
  m_Metadata = rhs.m_Metadata;
  return *this;
}

DataObject& DataObject::operator=(DataObject&& rhs) noexcept
{
  m_DataStructure = rhs.m_DataStructure;
  m_ParentList = std::move(rhs.m_ParentList);
  m_Id = rhs.m_Id;
  m_Name = std::move(rhs.m_Name);
  m_Metadata = std::move(rhs.m_Metadata);
  return *this;
}

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

DataObject::Type DataObject::getDataObjectType() const
{
  return Type::DataObject;
}

void DataObject::setId(IdType newId)
{
  m_Id = newId;
}

void DataObject::checkUpdatedIds(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  for(const auto& updatedId : updatedIds)
  {
    // Update parent list
    std::replace(m_ParentList.begin(), m_ParentList.end(), updatedId.first, updatedId.second);
  }

  // For derived classes
  checkUpdatedIdsImpl(updatedIds);
}

void DataObject::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
}

bool DataObject::AttemptToAddObject(DataStructure& dataStructure, const std::shared_ptr<DataObject>& data, const OptionalId& parentId)
{
  return dataStructure.finishAddingObject(data, parentId);
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

DataStructure& DataObject::getDataStructureRef()
{
  if(m_DataStructure == nullptr)
  {
    throw std::runtime_error("DataObject's DataStructure is null");
  }

  return *m_DataStructure;
}

const DataStructure& DataObject::getDataStructureRef() const
{
  if(m_DataStructure == nullptr)
  {
    throw std::runtime_error("DataObject's DataStructure is null");
  }

  return *m_DataStructure;
}

void DataObject::setDataStructure(DataStructure* dataStructure)
{
  m_DataStructure = dataStructure;
}

std::string DataObject::getName() const
{
  return m_Name;
}

int DataObject::canRename(const std::string& name) const
{

  if(name == getName())
  {
    return 1;
  }

  if(!IsValidName(name))
  {
    return 0;
  }

  const auto* dataStructPtr = getDataStructure();
  if(dataStructPtr == nullptr)
  {
    return 0;
  }

  return (std::any_of(m_ParentList.cbegin(), m_ParentList.cend(), [dataStructPtr, name](IdType parentId) {
    const auto* baseGroupPtr = dataStructPtr->getDataAs<BaseGroup>(parentId);
    if(baseGroupPtr == nullptr)
    {
      std::cout << "DataObject::canRename(name=" << name << ") cannot get baseGroup from parentId = " << parentId << std::endl;
    }
    return baseGroupPtr != nullptr && baseGroupPtr->contains(name);
  })) ? 2 : 0;
}

bool DataObject::rename(const std::string& name)
{
  if(canRename(name) != 1)
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

void DataObject::clearParents()
{
  m_ParentList.clear();
}

Metadata& DataObject::getMetadata()
{
  return m_Metadata;
}

const Metadata& DataObject::getMetadata() const
{
  return m_Metadata;
}

bool DataObject::hasParent(const DataPath& parentPath) const
{
  const auto dataPaths = getDataPaths();
  const auto originalCellDataPathIt = std::find_if(dataPaths.begin(), dataPaths.end(), [parentPath](const DataPath& path) {
    DataPath pathAncestor = path.getParent();
    while(!pathAncestor.empty())
    {
      if(parentPath == pathAncestor)
      {
        return true;
      }
      pathAncestor = pathAncestor.getParent();
    }
    return false;
  });
  return originalCellDataPathIt != dataPaths.end();
}

std::set<std::string> DataObject::StringListFromDataObjectType(const std::set<Type>& dataObjectTypes)
{
  static const std::map<Type, std::string> k_TypeToStringMap = {{Type::DataObject, "DataObject"},
                                                                {Type::DynamicListArray, "DynamicListArray"},
                                                                {Type::ScalarData, "ScalarData"},
                                                                {Type::BaseGroup, "BaseGroup"},
                                                                {Type::AbstractMontage, "AbstractMontage"},
                                                                {Type::DataGroup, "DataGroup"},
                                                                {Type::AttributeMatrix, "AttributeMatrix"},
                                                                {Type::IDataArray, "IDataArray"},
                                                                {Type::DataArray, "DataArray"},
                                                                {Type::IGeometry, "IGeometry"},
                                                                {Type::IGridGeometry, "IGridGeometry"},
                                                                {Type::RectGridGeom, "RectGridGeom"},
                                                                {Type::ImageGeom, "ImageGeom"},
                                                                {Type::INodeGeometry0D, "INodeGeometry0D"},
                                                                {Type::VertexGeom, "VertexGeom"},
                                                                {Type::INodeGeometry1D, "INodeGeometry1D"},
                                                                {Type::EdgeGeom, "EdgeGeom"},
                                                                {Type::INodeGeometry2D, "INodeGeometry2D"},
                                                                {Type::QuadGeom, "QuadGeom"},
                                                                {Type::TriangleGeom, "TriangleGeom"},
                                                                {Type::INodeGeometry3D, "INodeGeometry3D"},
                                                                {Type::HexahedralGeom, "HexahedralGeom"},
                                                                {Type::TetrahedralGeom, "TetrahedralGeom"},
                                                                {Type::INeighborList, "INeighborList"},
                                                                {Type::NeighborList, "NeighborList"},
                                                                {Type::StringArray, "StringArray"},
                                                                {Type::Unknown, "Unknown"},
                                                                {Type::Any, "Any"}};

  std::set<std::string> stringValues;
  for(auto objType : dataObjectTypes)
  {
    stringValues.insert(k_TypeToStringMap.at(objType));
  }
  return stringValues;
}

void DataObject::addParent(BaseGroup* parent)
{
  IdType identifier = parent->getId();
  if(std::find(m_ParentList.cbegin(), m_ParentList.cend(), identifier) != m_ParentList.cend())
  {
    return;
  }
  m_ParentList.push_back(identifier);
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
  if(getDataStructure() == nullptr)
  {
    return {};
  }

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

void DataObject::flush() const
{
}

uint64 DataObject::memoryUsage() const
{
  return 0;
}
} // namespace nx::core
