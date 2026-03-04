#include "AbstractNeighborList.hpp"

namespace nx::core
{
AbstractNeighborList::AbstractNeighborList(DataStructure& dataStructure, const std::string& name)
: AbstractArray(dataStructure, name)
{
}

AbstractNeighborList::AbstractNeighborList(DataStructure& dataStructure, const std::string& name, IdType importId)
: AbstractArray(dataStructure, name, importId)
{
}

AbstractNeighborList::~AbstractNeighborList() noexcept = default;

std::string AbstractNeighborList::getTypeName() const
{
  return NeighborListConstants::k_TypeName;
}

AbstractDataObject::Type AbstractNeighborList::getDataObjectType() const
{
  return Type::AbstractNeighborList;
}

void AbstractNeighborList::setNumNeighborsArrayName(const std::string& name)
{
  m_NumNeighborsArrayName = name;
}

std::string AbstractNeighborList::getNumNeighborsArrayName() const
{
  std::string arrayName = m_NumNeighborsArrayName;
  if(arrayName.empty())
  {
    return getName() + "_NumNeighbors";
  }
  return arrayName;
}

IListStore& AbstractNeighborList::getIListStoreRef()
{
  IListStore* store = getIListStore();
  if(store == nullptr)
  {
    throw std::runtime_error("AbstractNeighborList: Null IListStore");
  }
  return *store;
}

const IListStore& AbstractNeighborList::getIListStoreRef() const
{
  const IListStore* store = getIListStore();
  if(store == nullptr)
  {
    throw std::runtime_error("AbstractNeighborList: Null IListStore");
  }
  return *store;
}

usize AbstractNeighborList::getNumberOfTuples() const
{
  return getIListStoreRef().getNumberOfTuples();
}

void AbstractNeighborList::resizeTuples(const ShapeType& tupleShape)
{
  getIListStoreRef().resizeTuples(tupleShape);
}

usize AbstractNeighborList::getNumberOfComponents() const
{
  return 1;
}

ShapeType AbstractNeighborList::getTupleShape() const
{
  return getIListStoreRef().getTupleShape();
}

ShapeType AbstractNeighborList::getComponentShape() const
{
  return {1};
}

} // namespace nx::core
