#include "INeighborList.hpp"

#include "simplnx/DataStructure/NeighborList.hpp"

namespace nx::core
{
INeighborList::INeighborList(DataStructure& dataStructure, const std::string& name, usize numTuples)
: IArray(dataStructure, name)
, m_NumTuples(numTuples)
{
}

INeighborList::INeighborList(DataStructure& dataStructure, const std::string& name, usize numTuples, IdType importId)
: IArray(dataStructure, name, importId)
, m_NumTuples(numTuples)
{
}

INeighborList::~INeighborList() noexcept = default;

std::string INeighborList::getTypeName() const
{
  return NeighborListConstants::k_TypeName;
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

usize INeighborList::getNumberOfTuples() const
{
  return m_NumTuples;
}

void INeighborList::setNumberOfTuples(usize numTuples)
{
  m_NumTuples = numTuples;
}

usize INeighborList::getNumberOfComponents() const
{
  return 1;
}

IArray::ShapeType INeighborList::getTupleShape() const
{
  return {m_NumTuples};
}

IArray::ShapeType INeighborList::getComponentShape() const
{
  return {1};
}

} // namespace nx::core
