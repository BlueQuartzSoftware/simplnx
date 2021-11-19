#include "AbstractMontage.hpp"

#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"

using namespace complex;

AbstractMontage::AbstractMontage(DataStructure& ds, std::string name)
: BaseGroup(ds, std::move(name))
{
}

AbstractMontage::AbstractMontage(DataStructure& ds, std::string name, IdType importId)
: BaseGroup(ds, std::move(name), importId)
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

AbstractMontage::CollectionType AbstractMontage::getGeometries() const
{
  CollectionType geometries;
  for(auto geometry : m_Collection)
  {
    if(geometry != nullptr)
    {
      geometries.push_back(geometry);
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
  for(auto geometry : getGeometries())
  {
    names.push_back(geometry->getName());
  }
  return names;
}

std::vector<LinkedPath> AbstractMontage::getLinkedPaths() const
{
  std::vector<DataPath> paths;
  for(auto geometry : getGeometries())
  {
  }

  std::vector<LinkedPath> linkedPaths;
  return linkedPaths;
}

AbstractMontage::Iterator AbstractMontage::begin()
{
  return getCollection().begin();
}

AbstractMontage::Iterator AbstractMontage::end()
{
  return getCollection().end();
}

AbstractMontage::ConstIterator AbstractMontage::begin() const
{
  return getCollection().begin();
}

AbstractMontage::ConstIterator AbstractMontage::end() const
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
  if(!dynamic_cast<const AbstractGeometry*>(obj))
  {
    return false;
  }
  return BaseGroup::canInsert(obj);
}
