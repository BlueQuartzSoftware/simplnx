#include "AbstractMontage.hpp"

#include "simplnx/DataStructure/Geometry/IGeometry.hpp"

using namespace nx::core;

AbstractMontage::AbstractMontage(DataStructure& dataStructure, std::string name)
: BaseGroup(dataStructure, std::move(name))
{
}

AbstractMontage::AbstractMontage(DataStructure& dataStructure, std::string name, IdType importId)
: BaseGroup(dataStructure, std::move(name), importId)
{
}

AbstractMontage::AbstractMontage(const AbstractMontage& other)
: BaseGroup(other)
, m_Collection(other.m_Collection)
{
}

AbstractMontage::AbstractMontage(AbstractMontage&& other) noexcept
: BaseGroup(std::move(other))
, m_Collection(std::move(other.m_Collection))
{
}

AbstractMontage::~AbstractMontage() = default;

DataObject::Type AbstractMontage::getDataObjectType() const
{
  return Type::AbstractMontage;
}

AbstractMontage::CollectionType AbstractMontage::getGeometries() const
{
  CollectionType geometries;
  for(auto* geometryPtr : m_Collection)
  {
    if(geometryPtr != nullptr)
    {
      geometries.push_back(geometryPtr);
    }
  }
  return geometries;
}

usize AbstractMontage::getTileCount() const
{
  return m_Collection.size();
}

std::vector<std::string> AbstractMontage::getGeometryNames() const
{
  std::vector<std::string> names;
  for(auto* geometryPtr : getGeometries())
  {
    names.push_back(geometryPtr->getName());
  }
  return names;
}

std::vector<LinkedPath> AbstractMontage::GetLinkedPaths()
{
  std::vector<LinkedPath> linkedPaths;
  return linkedPaths;
}

AbstractMontage::iterator AbstractMontage::begin()
{
  return getCollection().begin();
}

AbstractMontage::iterator AbstractMontage::end()
{
  return getCollection().end();
}

AbstractMontage::const_iterator AbstractMontage::begin() const
{
  return getCollection().begin();
}

AbstractMontage::const_iterator AbstractMontage::end() const
{
  return getCollection().end();
}

AbstractMontage::CollectionType& AbstractMontage::getCollection()
{
  return m_Collection;
}

const AbstractMontage::CollectionType& AbstractMontage::getCollection() const
{
  return m_Collection;
}

bool AbstractMontage::canInsert(const DataObject* obj) const
{
  if(dynamic_cast<const IGeometry*>(obj) == nullptr)
  {
    return false;
  }
  return BaseGroup::canInsert(obj);
}
