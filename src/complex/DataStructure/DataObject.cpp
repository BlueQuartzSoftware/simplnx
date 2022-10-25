#include "DataObject.hpp"

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5ObjectWriter.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <algorithm>
#include <stdexcept>

using namespace complex;

namespace complex
{
bool DataObject::IsValidName(std::string_view name)
{
  return !name.empty() && name.find('/') == std::string_view::npos;
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
  m_Id = rhs.m_Id;
  m_Name = rhs.m_Name;
  return *this;
}

DataObject& DataObject::operator=(DataObject&& rhs) noexcept
{
  m_DataStructure = rhs.m_DataStructure;
  m_Id = rhs.m_Id;
  m_Name = std::move(rhs.m_Name);
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
  if(dataStruct == nullptr)
  {
    return false;
  }
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

bool DataObject::hasParent(const DataPath& parentPath) const
{
  const auto dataPaths = getDataPaths();
  const auto originalCellDataPathIt =
      std::find_if(dataPaths.begin(), dataPaths.end(), [parentPath](const DataPath& path) { return StringUtilities::contains(path.toString(), parentPath.toString()); });
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
  IdType id = parent->getId();
  if(std::find(m_ParentList.cbegin(), m_ParentList.cend(), id) != m_ParentList.cend())
  {
    return;
  }
  m_ParentList.push_back(id);
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

H5::ErrorType DataObject::writeH5ObjectAttributes(H5::DataStructureWriter& dataStructureWriter, H5::ObjectWriter& objectWriter, bool importable) const
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
  if(error < 0)
  {
    return error;
  }

  auto importableAttributeWriter = objectWriter.createAttribute(complex::Constants::k_ImportableTag);
  error = importableAttributeWriter.writeValue<int32>(importable ? 1 : 0);

  return error;
}
} // namespace complex
