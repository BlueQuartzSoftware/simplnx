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
  return neighbor_list_constants::k_TypeName;
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
  IListStore* storePtr = getIListStore();
  if(storePtr == nullptr)
  {
    throw std::runtime_error("INeighborList: Null IListStore");
  }
  return *storePtr;
}

const IListStore& INeighborList::getIListStoreRef() const
{
  const IListStore* storePtr = getIListStore();
  if(storePtr == nullptr)
  {
    throw std::runtime_error("INeighborList: Null IListStore");
  }
  return *storePtr;
}

usize INeighborList::getNumberOfTuples() const
{
  return getIListStoreRef().getNumberOfTuples();
}

Result<> INeighborList::resizeTuples(const ShapeType& tupleShape)
{
  return getIListStoreRef().resizeTuples(tupleShape);
}

usize INeighborList::getNumberOfComponents() const
{
  return 1;
}

ShapeType INeighborList::getTupleShape() const
{
  return getIListStoreRef().getTupleShape();
}

ShapeType INeighborList::getComponentShape() const
{
  return {1};
}

} // namespace nx::core
