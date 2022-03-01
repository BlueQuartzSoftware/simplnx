#include "INeighborList.hpp"

namespace complex
{
INeighborList::INeighborList(DataStructure& dataStructure, const std::string& name, usize numTuples)
: DataObject(dataStructure, name)
, m_NumTuples(numTuples)
{
}

INeighborList::INeighborList(DataStructure& dataStructure, const std::string& name, usize numTuples, IdType importId)
: DataObject(dataStructure, name, importId)
, m_NumTuples(numTuples)
{
}

INeighborList::~INeighborList() = default;

std::string INeighborList::getTypeName() const
{
  return std::string("NeighborList<T>");
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
} // namespace complex
