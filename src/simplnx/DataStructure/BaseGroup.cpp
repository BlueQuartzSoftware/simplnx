#include "BaseGroup.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

using namespace nx::core;

BaseGroup::BaseGroup(DataStructure& dataStructure, std::string name)
: AbstractDataObject(dataStructure, std::move(name))
{
}

BaseGroup::BaseGroup(DataStructure& dataStructure, std::string name, IdType importId)
: AbstractDataObject(dataStructure, std::move(name), importId)
{
}

BaseGroup::BaseGroup(const BaseGroup& other)
: AbstractDataObject(other)
, m_DataMap(other.m_DataMap)
{
}

BaseGroup::BaseGroup(BaseGroup&& other)
: AbstractDataObject(std::move(other))
, m_DataMap(std::move(other.m_DataMap))
{
  auto keys = m_DataMap.getKeys();
  for(auto& key : keys)
  {
    m_DataMap[key]->replaceParent(&other, this);
  }
}

BaseGroup::~BaseGroup() = default;

AbstractDataObject::Type BaseGroup::getDataObjectType() const
{
  return Type::BaseGroup;
}

bool BaseGroup::isGroup() const
{
  return true;
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

bool BaseGroup::contains(const AbstractDataObject* obj) const
{
  return m_DataMap.contains(obj);
}

AbstractDataObject* BaseGroup::operator[](const std::string& name)
{
  return m_DataMap[name];
}

const AbstractDataObject* BaseGroup::operator[](const std::string& name) const
{
  return m_DataMap[name];
}

AbstractDataObject& BaseGroup::at(const std::string& name)
{
  return m_DataMap.at(name);
}

const AbstractDataObject& BaseGroup::at(const std::string& name) const
{
  return m_DataMap.at(name);
}

bool BaseGroup::canInsert(const AbstractDataObject* obj) const
{
  if(obj == nullptr)
  {
    return false;
  }
  if(contains(obj) || contains(obj->getName()))
  {
    return false;
  }
  if(const auto* objGroup = dynamic_cast<const BaseGroup*>(obj); objGroup != nullptr && objGroup->isParentOf(this))
  {
    return false;
  }
  return true;
}

void BaseGroup::setDataStructure(DataStructure* dataStructure)
{
  AbstractDataObject::setDataStructure(dataStructure);
  m_DataMap.setDataStructure(dataStructure);
}

BaseGroup::Iterator BaseGroup::find(const std::string& name)
{
  return m_DataMap.find(name);
}

BaseGroup::ConstIterator BaseGroup::find(const std::string& name) const
{
  return m_DataMap.find(name);
}

bool BaseGroup::isParentOf(const AbstractDataObject* dataObj) const
{
  const std::vector<DataPath> origDataPaths = getDataPaths();
  return std::find_if(origDataPaths.begin(), origDataPaths.end(), [dataObj](const DataPath& path) { return dataObj->hasParent(path); }) != origDataPaths.end();
}

bool BaseGroup::insert(const std::weak_ptr<AbstractDataObject>& obj)
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

bool BaseGroup::remove(AbstractDataObject* obj)
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

const std::set<BaseGroup::GroupType>& BaseGroup::GetAllGeometryGroupTypes()
{
  static const std::set<GroupType> types = {GroupType::ImageGeom,    GroupType::RectGridGeom, GroupType::VertexGeom,      GroupType::EdgeGeom,
                                            GroupType::TriangleGeom, GroupType::QuadGeom,     GroupType::TetrahedralGeom, GroupType::HexahedralGeom};
  return types;
}

std::set<std::string> BaseGroup::StringListFromGroupType(const std::set<GroupType>& groupTypes)
{
  static const std::map<GroupType, std::string> k_TypeToStringMap = {{GroupType::BaseGroup, "BaseGroup"},
                                                                     {GroupType::DataGroup, "DataGroup"},
                                                                     {GroupType::AttributeMatrix, "AttributeMatrix"},
                                                                     {GroupType::AbstractGeometry, "AbstractGeometry"},
                                                                     {GroupType::AbstractGridGeometry, "AbstractGridGeometry"},
                                                                     {GroupType::RectGridGeom, "RectGridGeom"},
                                                                     {GroupType::ImageGeom, "ImageGeom"},
                                                                     {GroupType::AbstractNodeGeometry0D, "AbstractNodeGeometry0D"},
                                                                     {GroupType::VertexGeom, "VertexGeom"},
                                                                     {GroupType::AbstractNodeGeometry1D, "AbstractNodeGeometry1D"},
                                                                     {GroupType::EdgeGeom, "EdgeGeom"},
                                                                     {GroupType::AbstractNodeGeometry2D, "AbstractNodeGeometry2D"},
                                                                     {GroupType::QuadGeom, "QuadGeom"},
                                                                     {GroupType::TriangleGeom, "TriangleGeom"},
                                                                     {GroupType::AbstractNodeGeometry3D, "AbstractNodeGeometry3D"},
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

void BaseGroup::checkUpdatedIdsImpl(const std::unordered_map<AbstractDataObject::IdType, AbstractDataObject::IdType>& updatedIdsMap)
{
  m_DataMap.updateIds(updatedIdsMap);
}

std::vector<AbstractDataObject::IdType> BaseGroup::GetChildrenIds()
{
  return m_DataMap.getKeys();
}