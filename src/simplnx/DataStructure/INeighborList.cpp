#include "INeighborList.hpp"

namespace nx::core
{
INeighborList::INeighborList(DataStructure& dataStructure, const std::string& name)
: IArray(dataStructure, name)
{
}

INeighborList::INeighborList(DataStructure& dataStructure, const std::string& name, IdType importId)
: IArray(dataStructure, name, importId)
{
}

INeighborList::~INeighborList() noexcept = default;

std::string INeighborList::getTypeName() const
{
  return NeighborListConstants::k_TypeName;
}

DataObject::Type INeighborList::getDataObjectType() const
{
  return Type::INeighborList;
}

void INeighborList::setNumNeighborsArrayName(const std::string& name)
{
  m_NumNeighborsArrayName = name;
}

std::string INeighborList::getNumNeighborsArrayName() const
{
  std::string arrayName = m_NumNeighborsArrayName;
  if(arrayName.empty())
  {
    return getName() + "_NumNeighbors";
  }
  return arrayName;
}

IListStore& INeighborList::getIListStoreRef()
{
  IListStore* store = getIListStore();
  if(store == nullptr)
  {
    throw std::runtime_error("INeighborList: Null IListStore");
  }
  return *store;
}

const IListStore& INeighborList::getIListStoreRef() const
{
  const IListStore* store = getIListStore();
  if(store == nullptr)
  {
    throw std::runtime_error("INeighborList: Null IListStore");
  }
  return *store;
}

usize INeighborList::getNumberOfTuples() const
{
  return getIListStoreRef().getNumberOfTuples();
}

void INeighborList::resizeTuples(const std::vector<usize>& tupleShape)
{
  getIListStoreRef().resizeTuples(tupleShape);
}

usize INeighborList::getNumberOfComponents() const
{
  return 1;
}

IArray::ShapeType INeighborList::getTupleShape() const
{
  return getIListStoreRef().getTupleShape();
}

IArray::ShapeType INeighborList::getComponentShape() const
{
  return {1};
}

} // namespace nx::core
