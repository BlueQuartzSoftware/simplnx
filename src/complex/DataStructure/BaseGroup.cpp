#include "BaseGroup.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupWriter.hpp"
#include "complex/Utilities/StringUtilities.hpp"

using namespace complex;

BaseGroup::BaseGroup(DataStructure& dataGraph, std::string name)
: DataObject(dataGraph, std::move(name))
{
}

BaseGroup::BaseGroup(DataStructure& dataGraph, std::string name, IdType importId)
: DataObject(dataGraph, std::move(name), importId)
{
}

BaseGroup::BaseGroup(const BaseGroup& other)
: DataObject(other)
, m_DataMap(other.m_DataMap)
{
}

BaseGroup::BaseGroup(BaseGroup&& other)
: DataObject(std::move(other))
, m_DataMap(std::move(other.m_DataMap))
{
  auto keys = m_DataMap.getKeys();
  for(auto& key : keys)
  {
    m_DataMap[key]->replaceParent(&other, this);
  }
}

BaseGroup::~BaseGroup() = default;

DataObject::Type BaseGroup::getDataObjectType() const
{
  return Type::BaseGroup;
}

BaseGroup::GroupType BaseGroup::getGroupType() const
{
  return GroupType::BaseGroup;
}

const DataMap& BaseGroup::getDataMap() const
{
  return m_DataMap;
}

usize BaseGroup::getSize() const
{
  return m_DataMap.getSize();
}

bool BaseGroup::empty() const
{
  return m_DataMap.empty();
}

DataMap& BaseGroup::getDataMap()
{
  return m_DataMap;
}

bool BaseGroup::contains(const std::string& name) const
{
  return m_DataMap.contains(name);
}

bool BaseGroup::contains(const DataObject* obj) const
{
  return m_DataMap.contains(obj);
}

DataObject* BaseGroup::operator[](const std::string& name)
{
  return m_DataMap[name];
}

const DataObject* BaseGroup::operator[](const std::string& name) const
{
  return m_DataMap[name];
}

DataObject& BaseGroup::at(const std::string& name)
{
  return m_DataMap.at(name);
}

const DataObject& BaseGroup::at(const std::string& name) const
{
  return m_DataMap.at(name);
}

bool BaseGroup::canInsert(const DataObject* obj) const
{
  if(obj == nullptr)
  {
    return false;
  }
  if(contains(obj) || contains(obj->getName()))
  {
    return false;
  }
  return true;
}

void BaseGroup::setDataStructure(DataStructure* dataGraph)
{
  DataObject::setDataStructure(dataGraph);
  m_DataMap.setDataStructure(dataGraph);
}

BaseGroup::Iterator BaseGroup::find(const std::string& name)
{
  return m_DataMap.find(name);
}

BaseGroup::ConstIterator BaseGroup::find(const std::string& name) const
{
  return m_DataMap.find(name);
}

bool BaseGroup::isParentOf(const DataObject* dataObj) const
{
  const auto origDataPaths = getDataPaths();
  return std::find_if(origDataPaths.begin(), origDataPaths.end(), [dataObj](const DataPath& path) { return dataObj->hasParent(path); }) != origDataPaths.end();
}

bool BaseGroup::insert(const std::weak_ptr<DataObject>& obj)
{
  auto ptr = obj.lock();
  if(!canInsert(ptr.get()))
  {
    return false;
  }
  if(m_DataMap.insert(ptr))
  {
    ptr->addParent(this);
    return true;
  }
  return false;
}

bool BaseGroup::remove(DataObject* obj)
{
  if(obj == nullptr)
  {
    return false;
  }
  obj->removeParent(this);
  return m_DataMap.remove(obj->getId());
}

bool BaseGroup::remove(const std::string& name)
{
  for(auto iter = m_DataMap.begin(); iter != m_DataMap.end(); iter++)
  {
    if((*iter).second->getName() == name)
    {
      (*iter).second->removeParent(this);
      m_DataMap.erase(iter);
      return true;
    }
  }
  return false;
}

void BaseGroup::clear()
{
  m_DataMap.clear();
}

BaseGroup::Iterator BaseGroup::begin()
{
  return m_DataMap.begin();
}
BaseGroup::Iterator BaseGroup::end()
{
  return m_DataMap.end();
}
BaseGroup::ConstIterator BaseGroup::begin() const
{
  return m_DataMap.begin();
}
BaseGroup::ConstIterator BaseGroup::end() const
{
  return m_DataMap.end();
}

const std::set<BaseGroup::GroupType>& BaseGroup::GetAllGroupTypes()
{
  static const std::set<GroupType> types = {GroupType::DataGroup, GroupType::AttributeMatrix, GroupType::ImageGeom, GroupType::RectGridGeom,    GroupType::VertexGeom,
                                            GroupType::EdgeGeom,  GroupType::TriangleGeom,    GroupType::QuadGeom,  GroupType::TetrahedralGeom, GroupType::HexahedralGeom};
  return types;
}

std::set<std::string> BaseGroup::StringListFromGroupType(const std::set<GroupType>& groupTypes)
{
  static const std::map<GroupType, std::string> k_TypeToStringMap = {{GroupType::BaseGroup, "BaseGroup"},
                                                                     {GroupType::DataGroup, "DataGroup"},
                                                                     {GroupType::AttributeMatrix, "AttributeMatrix"},
                                                                     {GroupType::IGeometry, "IGeometry"},
                                                                     {GroupType::IGridGeometry, "IGridGeometry"},
                                                                     {GroupType::RectGridGeom, "RectGridGeom"},
                                                                     {GroupType::ImageGeom, "ImageGeom"},
                                                                     {GroupType::INodeGeometry0D, "INodeGeometry0D"},
                                                                     {GroupType::VertexGeom, "VertexGeom"},
                                                                     {GroupType::INodeGeometry1D, "INodeGeometry1D"},
                                                                     {GroupType::EdgeGeom, "EdgeGeom"},
                                                                     {GroupType::INodeGeometry2D, "INodeGeometry2D"},
                                                                     {GroupType::QuadGeom, "QuadGeom"},
                                                                     {GroupType::TriangleGeom, "TriangleGeom"},
                                                                     {GroupType::INodeGeometry3D, "INodeGeometry3D"},
                                                                     {GroupType::HexahedralGeom, "HexahedralGeom"},
                                                                     {GroupType::TetrahedralGeom, "TetrahedralGeom"},
                                                                     {GroupType::Unknown, "Unknown"}};

  std::set<std::string> stringValues;
  for(auto groupType : groupTypes)
  {
    stringValues.insert(k_TypeToStringMap.at(groupType));
  }
  return stringValues;
}

std::vector<std::string> BaseGroup::GetChildrenNames()
{
  return m_DataMap.getNames();
}

H5::ErrorType BaseGroup::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  return m_DataMap.readH5Group(dataStructureReader, groupReader, getId(), preflight);
}

H5::ErrorType BaseGroup::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  auto error = writeH5ObjectAttributes(dataStructureWriter, groupWriter, importable);
  if(error < 0)
  {
    return error;
  }

  return m_DataMap.writeH5Group(dataStructureWriter, groupWriter);
}

void BaseGroup::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  m_DataMap.updateIds(updatedIds);
}
