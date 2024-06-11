#include "NeighborList.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/ListStore.hpp"

namespace nx::core
{
template <typename T>
NeighborList<T>::NeighborList(DataStructure& dataStructure, const std::string& name, usize numTuples)
: INeighborList(dataStructure, name, numTuples)
, m_Store(std::make_shared<ListStore<T>>(std::vector<uint64>{numTuples}))
, m_IsAllocated(false)
, m_InitValue(static_cast<T>(0.0))
{
}

template <typename T>
NeighborList<T>::NeighborList(DataStructure& dataStructure, const std::string& name, const std::vector<SharedVectorType>& dataVector, IdType importId)
: INeighborList(dataStructure, name, dataVector.size(), importId)
, m_Store(std::make_shared<ListStore<T>>(dataVector))
, m_IsAllocated(true)
, m_InitValue(static_cast<T>(0.0))
{
}

template <typename T>
NeighborList<T>* NeighborList<T>::Create(DataStructure& dataStructure, const std::string& name, usize numTuples, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<NeighborList>(new NeighborList(dataStructure, name, numTuples));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

template <typename T>
NeighborList<T>* NeighborList<T>::Import(DataStructure& dataStructure, const std::string& name, IdType importId, const std::vector<SharedVectorType>& dataVector, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<NeighborList>(new NeighborList(dataStructure, name, dataVector, importId));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

template <typename T>
DataObject* NeighborList<T>::shallowCopy()
{
  return new NeighborList(*this);
}

template <typename T>
std::shared_ptr<DataObject> NeighborList<T>::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = getDataStructureRef();
  if(dataStruct.containsData(copyPath))
  {
    return nullptr;
  }
  // Don't construct with identifier since it will get created when inserting into data structure
  auto copy = std::shared_ptr<NeighborList<T>>(new NeighborList<T>(dataStruct, copyPath.getTargetName(), getNumberOfTuples()));
  copy->setNumNeighborsArrayName(getNumNeighborsArrayName());
  copy->m_Store->copy(*m_Store.get());
  if(dataStruct.insert(copy, copyPath.getParent()))
  {
    return copy;
  }
  return nullptr;
}

template <typename T>
void NeighborList<T>::setInitValue(value_type initValue)
{
  m_InitValue = initValue;
}

template <typename T>
int32 NeighborList<T>::eraseTuples(const std::vector<usize>& idxs)
{
  int32 err = 0;
  // If nothing is to be erased just return
  if(idxs.empty())
  {
    return 0;
  }

  usize idxsSize = static_cast<usize>(idxs.size());
  if(idxsSize >= getNumberOfTuples())
  {
    resizeTuples(0);
    return 0;
  }

  usize arraySize = m_Store->size();
  // Sanity Check the Indices in the vector to make sure we are not trying to remove any indices that are
  // off the end of the array and return an error code.
  for(usize idx : idxs)
  {
    if(idx >= arraySize)
    {
      return -100;
    }
  }

  auto copy = m_Store->deepCopy();
  copy->resizeTuples(arraySize - idxsSize);

  usize idxsIndex = 0;
  usize rIdx = 0;
  for(usize dIdx = 0; dIdx < arraySize; ++dIdx)
  {
    if(dIdx != idxs[idxsIndex])
    {
      copy->setList(rIdx, m_Store->at(dIdx));
      ++rIdx;
    }
    else
    {
      ++idxsIndex;
      if(idxsIndex == idxsSize)
      {
        idxsIndex--;
      }
    }
  }
  setNumberOfTuples(m_Store->size());
  m_Store = std::move(copy);
  return err;
}

template <typename T>
void NeighborList<T>::copyTuple(usize currentPos, usize newPos)
{
  m_Store->setList(newPos, m_Store->at(currentPos));
}

template <typename T>
usize NeighborList<T>::getSize() const
{
  usize total = 0;
  for(usize dIdx = 0; dIdx < m_Store->size(); ++dIdx)
  {
    total += m_Store->getListSize(dIdx);
  }
  return total;
}

template <typename T>
usize NeighborList<T>::size() const
{
  usize total = 0;
  for(usize dIdx = 0; dIdx < m_Store->size(); ++dIdx)
  {
    total += m_Store->getListSize(dIdx);
  }
  return total;
}

template <typename T>
bool NeighborList<T>::empty() const
{
  return getNumberOfTuples() == 0;
}

template <typename T>
void NeighborList<T>::setNumberOfComponents(int32 nc)
{
  throw std::runtime_error(fmt::format("{}:({}): NeighborLists do NOT have components", __FILE__, __LINE__));
}

template <typename T>
usize NeighborList<T>::getTypeSize() const
{
  return sizeof(SharedVectorType);
}

template <typename T>
void NeighborList<T>::initializeWithZeros()
{
  m_Store->clear();
  m_IsAllocated = false;
}

template <typename T>
int32 NeighborList<T>::resizeTotalElements(usize size)
{
  usize old = m_Store->size();
  m_Store->resizeTuples(size);
  setNumberOfTuples(size);

  if(size == 0)
  {
    m_IsAllocated = false;
  }
  else
  {
    m_IsAllocated = true;
  }
  return 1;
}

template <typename T>
void NeighborList<T>::resizeTuples(usize numTuples)
{
  resizeTotalElements(numTuples);
}

template <typename T>
void NeighborList<T>::addEntry(int32 grainId, value_type value)
{
  if(grainId >= static_cast<int32>(m_Store->size()))
  {
    usize old = m_Store->size();
    m_Store->resizeTuples(grainId + 1);
    m_IsAllocated = true;
    // Initialize with zero length Vectors
    for(usize i = old; i < m_Store->size(); ++i)
    {
      m_Store->setList(i, SharedVectorType(new VectorType));
    }
    setNumberOfTuples(m_Store->size());
  }
  m_Store->addEntry(grainId, value);
}

template <typename T>
void NeighborList<T>::clearAllLists()
{
  m_Store->clear();
  m_IsAllocated = false;
}

template <typename T>
void NeighborList<T>::setList(int32 grainId, const SharedVectorType& neighborList)
{
  if(grainId >= static_cast<int32>(m_Store->size()))
  {
    usize old = m_Store->size();
    m_Store->resizeTuples(grainId + 1);
    m_IsAllocated = true;
    // Initialize with zero length Vectors
    for(usize i = old; i < m_Store->size(); ++i)
    {
      m_Store->setList(i, SharedVectorType(new VectorType));
    }
  }
  m_Store->setList(grainId, neighborList);
}

template <typename T>
void NeighborList<T>::setLists(const std::vector<std::vector<T>>& neighborLists)
{
  usize totalFeatures = neighborLists.size();
  uint64 reserveSize = 0;
  for(size_t i = 1; i < totalFeatures; i++)
  {
    reserveSize = std::max(reserveSize, neighborLists[i].size());
  }
  reserveListSize(reserveSize);
  resizeTuples(totalFeatures);

  for(size_t i = 1; i < totalFeatures; i++)
  {
    // Set the vector for each list into the NeighborList Object
    SharedVectorType list(new std::vector<T>);
    list->assign(neighborLists[i].begin(), neighborLists[i].end());
    setList(static_cast<int32_t>(i), list);
  }
}

template <typename T>
void NeighborList<T>::reserveListSize(uint64 size)
{
  m_Store->setXtensorListSize(size);
}

template <typename T>
T NeighborList<T>::getValue(int32 grainId, int32 index, bool& ok) const
{
  VectorType vec = m_Store->at(grainId);
  if(index < 0 || static_cast<usize>(index) >= vec.size())
  {
    ok = false;
    return static_cast<T>(-1);
  }
  return vec[index];
}

template <typename T>
int32 NeighborList<T>::getNumberOfLists() const
{
  return static_cast<int32>(m_Store->size());
}

template <typename T>
int32 NeighborList<T>::getListSize(int32 grainId) const
{
  return static_cast<int32>(m_Store->getListSize(grainId));
}

template <typename T>
typename NeighborList<T>::VectorType NeighborList<T>::getList(int32 grainId) const
{
  return m_Store->at(grainId);
}

template <typename T>
typename NeighborList<T>::VectorType NeighborList<T>::copyOfList(int32 grainId) const
{
  VectorType copy(m_Store->at(grainId));
  return copy;
}

template <typename T>
typename NeighborList<T>::VectorType NeighborList<T>::operator[](int32 grainId)
{
  return m_Store->at(grainId);
}

template <typename T>
typename NeighborList<T>::VectorType NeighborList<T>::operator[](usize grainId)
{
  return m_Store->at(grainId);
}

template <typename T>
void NeighborList<T>::setValue(usize index, const VectorType& value)
{
  m_Store->setList(index, value);
}

template <typename T>
typename NeighborList<T>::VectorType NeighborList<T>::at(int32 grainId) const
{
  return m_Store->at(grainId);
}

template <typename T>
typename NeighborList<T>::VectorType NeighborList<T>::at(usize grainId) const
{
  return m_Store->at(grainId);
}

template <typename T>
DataObject::Type NeighborList<T>::getDataObjectType() const
{
  return Type::NeighborList;
}

template <typename T>
void NeighborList<T>::resizeTuples(const std::vector<usize>& tupleShape)
{
  auto numTuples = std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<usize>(1), std::multiplies<>());
  resizeTotalElements(numTuples);
}

template <typename T>
std::shared_ptr<typename NeighborList<T>::store_type> NeighborList<T>::getStore() const
{
  return m_Store;
}

template <typename T>
std::vector<typename NeighborList<T>::VectorType> NeighborList<T>::getVectors() const
{
  usize count = m_Store->size();
  std::vector<typename NeighborList<T>::VectorType> vectors(count);
  for (usize i = 0; i < count; i++)
  {
    vectors[i] = m_Store->at(i);
  }
  return vectors;
}

template <typename T>
typename NeighborList<T>::iterator NeighborList<T>::begin()
{
  return m_Store->begin();
}

template <typename T>
typename NeighborList<T>::iterator NeighborList<T>::end()
{
  return m_Store->end();
}

template <typename T>
typename NeighborList<T>::const_iterator NeighborList<T>::begin() const
{
  return m_Store->begin();
}

template <typename T>
typename NeighborList<T>::const_iterator NeighborList<T>::end() const
{
  return m_Store->end();
}

template <typename T>
typename NeighborList<T>::const_iterator NeighborList<T>::cbegin() const
{
  return m_Store->begin();
}

template <typename T>
typename NeighborList<T>::const_iterator NeighborList<T>::cend() const
{
  return m_Store->end();
}

template <>
DataType SIMPLNX_EXPORT NeighborList<int8>::getDataType() const
{
  return DataType::int8;
}

template <>
DataType SIMPLNX_EXPORT NeighborList<int16>::getDataType() const
{
  return DataType::int16;
}

template <>
DataType SIMPLNX_EXPORT NeighborList<int32>::getDataType() const
{
  return DataType::int32;
}

template <>
DataType SIMPLNX_EXPORT NeighborList<int64>::getDataType() const
{
  return DataType::int64;
}

template <>
DataType SIMPLNX_EXPORT NeighborList<uint8>::getDataType() const
{
  return DataType::uint8;
}

template <>
DataType SIMPLNX_EXPORT NeighborList<uint16>::getDataType() const
{
  return DataType::uint16;
}

template <>
DataType SIMPLNX_EXPORT NeighborList<uint32>::getDataType() const
{
  return DataType::uint32;
}

template <>
DataType SIMPLNX_EXPORT NeighborList<uint64>::getDataType() const
{
  return DataType::uint64;
}

template <>
DataType SIMPLNX_EXPORT NeighborList<float32>::getDataType() const
{
  return DataType::float32;
}

template <>
DataType SIMPLNX_EXPORT NeighborList<float64>::getDataType() const
{
  return DataType::float64;
}

template class SIMPLNX_TEMPLATE_EXPORT NeighborList<int8>;
template class SIMPLNX_TEMPLATE_EXPORT NeighborList<uint8>;

template class SIMPLNX_TEMPLATE_EXPORT NeighborList<int16>;
template class SIMPLNX_TEMPLATE_EXPORT NeighborList<uint16>;

template class SIMPLNX_TEMPLATE_EXPORT NeighborList<int32>;
template class SIMPLNX_TEMPLATE_EXPORT NeighborList<uint32>;

template class SIMPLNX_TEMPLATE_EXPORT NeighborList<int64>;
template class SIMPLNX_TEMPLATE_EXPORT NeighborList<uint64>;

template class SIMPLNX_TEMPLATE_EXPORT NeighborList<float32>;
template class SIMPLNX_TEMPLATE_EXPORT NeighborList<float64>;
} // namespace nx::core
