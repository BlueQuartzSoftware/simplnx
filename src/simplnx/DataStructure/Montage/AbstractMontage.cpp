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

AbstractMontage::AbstractMontage(AbstractMontage&& other)
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

Result<> AbstractMontage::canInsert(const DataObject* obj) const
{
  if(!dynamic_cast<const IGeometry*>(obj))
  {
    return MakeErrorResult<>(-1676, fmt::format("AbstractMontage::canInsert() Error: DataObject with name='{}' and type='{}' is not subclass of IGeometry", obj->getName(), obj->getTypeName()));
  }
  return BaseGroup::canInsert(obj);
}
