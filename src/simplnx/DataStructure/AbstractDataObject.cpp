#include "AbstractDataObject.hpp"

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <algorithm>
#include <stdexcept>

using namespace nx::core;

namespace nx::core
{
bool AbstractDataObject::IsValidName(std::string_view name)
{
  return !name.empty() && name.find('/') == std::string_view::npos;
}

AbstractDataObject::AbstractDataObject(DataStructure& dataStructure, std::string name)
: AbstractDataObject(dataStructure, std::move(name), dataStructure.generateId())
{
}

AbstractDataObject::AbstractDataObject(DataStructure& dataStructure, std::string name, IdType importId)
: m_DataStructure(&dataStructure)
, m_Id(importId)
, m_Name(std::move(name))
{
  if(!IsValidName(m_Name))
  {
    throw std::invalid_argument("AbstractDataObject names cannot contain \"/\"");
  }
}

AbstractDataObject::AbstractDataObject(const AbstractDataObject& rhs)
: m_DataStructure(rhs.m_DataStructure)
, m_ParentList(rhs.m_ParentList)
, m_Id(rhs.m_Id)
, m_Name(rhs.m_Name)
, m_Metadata(rhs.m_Metadata)
{
}

AbstractDataObject::AbstractDataObject(AbstractDataObject&& rhs)
: m_DataStructure(rhs.m_DataStructure)
, m_ParentList(std::move(rhs.m_ParentList))
, m_Id(rhs.m_Id)
, m_Name(std::move(rhs.m_Name))
, m_Metadata(std::move(rhs.m_Metadata))
{
}

AbstractDataObject& AbstractDataObject::operator=(const AbstractDataObject& rhs)
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

AbstractDataObject& AbstractDataObject::operator=(AbstractDataObject&& rhs) noexcept
{
  m_DataStructure = rhs.m_DataStructure;
  m_ParentList = std::move(rhs.m_ParentList);
  m_Id = rhs.m_Id;
  m_Name = std::move(rhs.m_Name);
  m_Metadata = std::move(rhs.m_Metadata);
  return *this;
}

AbstractDataObject::~AbstractDataObject() noexcept
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

AbstractDataObject::Type AbstractDataObject::getDataObjectType() const
{
  return Type::AbstractDataObject;
}

bool AbstractDataObject::isGroup() const
{
  return false;
}

void AbstractDataObject::setId(IdType newId)
{
  m_Id = newId;
}

void AbstractDataObject::checkUpdatedIds(const std::unordered_map<AbstractDataObject::IdType, AbstractDataObject::IdType>& updatedIdsMap)
{
  // Use std::transform to map IDs
  ParentCollectionType newParentList;
  std::transform(m_ParentList.begin(), m_ParentList.end(), std::back_inserter(newParentList), [&updatedIdsMap](uint64 id) -> uint64 {
    auto it = updatedIdsMap.find(id);
    return (it != updatedIdsMap.end()) ? it->second : id;
  });

  m_ParentList = newParentList;

  // For derived classes
  checkUpdatedIdsImpl(updatedIdsMap);
}

void AbstractDataObject::checkUpdatedIdsImpl(const std::unordered_map<AbstractDataObject::IdType, AbstractDataObject::IdType>& updatedIdsMap)
{
}

bool AbstractDataObject::AttemptToAddObject(DataStructure& dataStructure, const std::shared_ptr<AbstractDataObject>& data, const OptionalId& parentId)
{
  return dataStructure.finishAddingObject(data, parentId);
}

AbstractDataObject::IdType AbstractDataObject::getId() const
{
  return m_Id;
}

DataStructure* AbstractDataObject::getDataStructure()
{
  return m_DataStructure;
}

const DataStructure* AbstractDataObject::getDataStructure() const
{
  return m_DataStructure;
}

DataStructure& AbstractDataObject::getDataStructureRef()
{
  if(m_DataStructure == nullptr)
  {
    throw std::runtime_error("AbstractDataObject's DataStructure is null");
  }

  return *m_DataStructure;
}

const DataStructure& AbstractDataObject::getDataStructureRef() const
{
  if(m_DataStructure == nullptr)
  {
    throw std::runtime_error("AbstractDataObject's DataStructure is null");
  }

  return *m_DataStructure;
}

void AbstractDataObject::setDataStructure(DataStructure* dataStructure)
{
  m_DataStructure = dataStructure;
}

std::string AbstractDataObject::getName() const
{
  return m_Name;
}

bool AbstractDataObject::canRename(const std::string& name) const
{

  if(name == getName())
  {
    return true;
  }

  if(!IsValidName(name))
  {
    return false;
  }

  const auto* dataStructPtr = getDataStructure();
  if(dataStructPtr == nullptr)
  {
    return false;
  }

  return !std::any_of(m_ParentList.cbegin(), m_ParentList.cend(), [dataStructPtr, name](IdType parentId) {
    const auto* baseGroupPtr = dataStructPtr->getDataAs<BaseGroup>(parentId);
    if(baseGroupPtr == nullptr)
    {
      std::cout << "AbstractDataObject::canRename(name=" << name << ") cannot get baseGroup from parentId = " << parentId << std::endl;
    }
    return baseGroupPtr != nullptr && baseGroupPtr->contains(name);
  });
}

bool AbstractDataObject::rename(const std::string& name)
{
  if(!canRename(name))
  {
    return false;
  }

  m_Name = name;
  return true;
}

AbstractDataObject::ParentCollectionType AbstractDataObject::getParentIds() const
{
  return m_ParentList;
}

void AbstractDataObject::clearParents()
{
  m_ParentList.clear();
}

Metadata& AbstractDataObject::getMetadata()
{
  return m_Metadata;
}

const Metadata& AbstractDataObject::getMetadata() const
{
  return m_Metadata;
}

bool AbstractDataObject::hasParent(const DataPath& parentPath) const
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

std::set<std::string> AbstractDataObject::StringListFromDataObjectType(const std::set<Type>& dataObjectTypes)
{
  static const std::map<Type, std::string> k_TypeToStringMap = {{Type::AbstractDataObject, "AbstractDataObject"},
                                                                {Type::DynamicListArray, "DynamicListArray"},
                                                                {Type::ScalarData, "ScalarData"},
                                                                {Type::BaseGroup, "BaseGroup"},
                                                                {Type::AbstractMontage, "AbstractMontage"},
                                                                {Type::DataGroup, "DataGroup"},
                                                                {Type::AttributeMatrix, "AttributeMatrix"},
                                                                {Type::AbstractDataArray, "AbstractDataArray"},
                                                                {Type::DataArray, "DataArray"},
                                                                {Type::AbstractGeometry, "AbstractGeometry"},
                                                                {Type::AbstractGridGeometry, "AbstractGridGeometry"},
                                                                {Type::RectGridGeom, "RectGridGeom"},
                                                                {Type::ImageGeom, "ImageGeom"},
                                                                {Type::AbstractNodeGeometry0D, "AbstractNodeGeometry0D"},
                                                                {Type::VertexGeom, "VertexGeom"},
                                                                {Type::AbstractNodeGeometry1D, "AbstractNodeGeometry1D"},
                                                                {Type::EdgeGeom, "EdgeGeom"},
                                                                {Type::AbstractNodeGeometry2D, "AbstractNodeGeometry2D"},
                                                                {Type::QuadGeom, "QuadGeom"},
                                                                {Type::TriangleGeom, "TriangleGeom"},
                                                                {Type::AbstractNodeGeometry3D, "AbstractNodeGeometry3D"},
                                                                {Type::HexahedralGeom, "HexahedralGeom"},
                                                                {Type::TetrahedralGeom, "TetrahedralGeom"},
                                                                {Type::AbstractNeighborList, "AbstractNeighborList"},
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

void AbstractDataObject::addParent(BaseGroup* parent)
{
  IdType identifier = parent->getId();
  if(std::find(m_ParentList.cbegin(), m_ParentList.cend(), identifier) != m_ParentList.cend())
  {
    return;
  }
  m_ParentList.push_back(identifier);
}

void AbstractDataObject::removeParent(BaseGroup* parent)
{
  m_ParentList.remove(parent->getId());
}

void AbstractDataObject::replaceParent(BaseGroup* oldParent, BaseGroup* newParent)
{
  std::replace(m_ParentList.begin(), m_ParentList.end(), oldParent->getId(), newParent->getId());
}

std::vector<DataPath> AbstractDataObject::getDataPaths() const
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

void AbstractDataObject::flush() const
{
}

uint64 AbstractDataObject::memoryUsage() const
{
  return 0;
}
} // namespace nx::core
