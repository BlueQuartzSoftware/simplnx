#include "NeighborList.hpp"

#include "complex/Utilities/Parsing/HDF5/H5GroupWriter.hpp"

using namespace complex;

template <typename T>
NeighborList<T>::NeighborList(DataStructure& dataStructure, const std::string name, usize numTuples)
: IDataArray(dataStructure, name)
, m_NumTuples(numTuples)
, m_IsAllocated(false)
{
}

template <typename T>
NeighborList<T>* NeighborList<T>::Create(DataStructure& ds, const std::string& name, usize numTuples, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<NeighborList>(new NeighborList(ds, std::move(name), numTuples));
  if(!AttemptToAddObject(ds, data, parentId))
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
DataObject* NeighborList<T>::deepCopy()
{
  auto copy = new NeighborList(*this);
  copy->m_NumNeighborsArrayName = m_NumNeighborsArrayName;
  copy->m_Array = m_Array;
  return copy;
}

template <typename T>
std::string NeighborList<T>::getTypeName() const
{
  return std::string("NeighborList<T>");
}

template <typename T>
void NeighborList<T>::setNumNeighborsArrayName(const std::string& name)
{
  m_NumNeighborsArrayName = name;
}

template <typename T>
std::string NeighborList<T>::getNumNeighborsArrayName()
{
  return m_NumNeighborsArrayName;
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

  usize arraySize = m_Array.size();
  // Sanity Check the Indices in the vector to make sure we are not trying to remove any indices that are
  // off the end of the array and return an error code.
  for(std::vector<usize>::size_type i = 0; i < idxs.size(); ++i)
  {
    if(idxs[i] >= arraySize)
    {
      return -100;
    }
  }

  std::vector<SharedVectorType> replacement(arraySize - idxsSize);

  usize idxsIndex = 0;
  usize rIdx = 0;
  for(usize dIdx = 0; dIdx < arraySize; ++dIdx)
  {
    if(dIdx != idxs[idxsIndex])
    {
      replacement[rIdx] = m_Array[dIdx];
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
  m_Array = replacement;
  m_NumTuples = m_Array.size();
  return err;
}

template <typename T>
void NeighborList<T>::copyTuple(usize currentPos, usize newPos)
{
  m_Array[newPos] = m_Array[currentPos];
}

template <typename T>
usize NeighborList<T>::getNumberOfTuples() const
{
  return m_NumTuples;
}

template <typename T>
usize NeighborList<T>::getSize() const
{
  usize total = 0;
  for(usize dIdx = 0; dIdx < m_Array.size(); ++dIdx)
  {
    total += m_Array[dIdx]->size();
  }
  return total;
}

template <typename T>
void NeighborList<T>::setNumberOfComponents(int32 nc)
{
}

template <typename T>
usize NeighborList<T>::getNumberOfComponents() const
{
  return 1;
}

template <typename T>
std::vector<usize> NeighborList<T>::getComponentDimensions() const
{
  std::vector<usize> dims = {1};
  return dims;
}

template <typename T>
usize NeighborList<T>::getTypeSize() const
{
  return sizeof(SharedVectorType);
}

template <typename T>
void NeighborList<T>::initializeWithZeros()
{
  m_Array.clear();
  m_IsAllocated = false;
}

template <typename T>
int32 NeighborList<T>::resizeTotalElements(usize size)
{
  // std::cout << "NeighborList::resizeTotalElements(" << size << ")" << std::endl;
  usize old = m_Array.size();
  m_Array.resize(size);
  m_NumTuples = size;
  if(size == 0)
  {
    m_IsAllocated = false;
  }
  else
  {
    m_IsAllocated = true;
  }
  // Initialize with zero length Vectors
  for(usize i = old; i < m_Array.size(); ++i)
  {
    m_Array[i] = SharedVectorType(new VectorType);
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
  if(grainId >= static_cast<int>(m_Array.size()))
  {
    usize old = m_Array.size();
    m_Array.resize(grainId + 1);
    m_IsAllocated = true;
    // Initialize with zero length Vectors
    for(usize i = old; i < m_Array.size(); ++i)
    {
      m_Array[i] = SharedVectorType(new VectorType);
    }
  }
  m_Array[grainId]->push_back(value);
  m_NumTuples = m_Array.size();
}

template <typename T>
void NeighborList<T>::clearAllLists()
{
  m_Array.clear();
  m_IsAllocated = false;
}

template <typename T>
void NeighborList<T>::setList(int32 grainId, SharedVectorType neighborList)
{
  if(grainId >= static_cast<int>(m_Array.size()))
  {
    usize old = m_Array.size();
    m_Array.resize(grainId + 1);
    m_IsAllocated = true;
    // Initialize with zero length Vectors
    for(usize i = old; i < m_Array.size(); ++i)
    {
      m_Array[i] = SharedVectorType(new VectorType);
    }
  }
  m_Array[grainId] = neighborList;
}

template <typename T>
T NeighborList<T>::getValue(int32 grainId, int32 index, bool& ok) const
{
  SharedVectorType vec = m_Array[grainId];
  if(index < 0 || static_cast<usize>(index) >= vec->size())
  {
    ok = false;
    return -1;
  }
  return (*vec)[index];
}

template <typename T>
int32 NeighborList<T>::getNumberOfLists() const
{
  return static_cast<int>(m_Array.size());
}

template <typename T>
int32 NeighborList<T>::getListSize(int32 grainId) const
{
  return static_cast<int>(m_Array[grainId]->size());
}

template <typename T>
typename NeighborList<T>::VectorType& NeighborList<T>::getListReference(int32 grainId) const
{
  return *(m_Array[grainId]);
}

template <typename T>
typename NeighborList<T>::SharedVectorType NeighborList<T>::getList(int32 grainId) const
{
  return m_Array[grainId];
}

template <typename T>
typename NeighborList<T>::VectorType NeighborList<T>::copyOfList(int32 grainId) const
{
  VectorType copy(*(m_Array[grainId]));
  return copy;
}

template <typename T>
typename NeighborList<T>::VectorType& NeighborList<T>::operator[](int32 grainId)
{
  return *(m_Array[grainId]);
}

template <typename T>
typename NeighborList<T>::VectorType& NeighborList<T>::operator[](usize grainId)
{
  return *(m_Array[grainId]);
}

template <typename T>
IDataStore* NeighborList<T>::getIDataStore()
{
  return nullptr;
}

template <typename T>
const IDataStore* NeighborList<T>::getIDataStore() const
{
  return nullptr;
}

template <typename T>
H5::ErrorType NeighborList<T>::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter) const
{
  auto datasetWriter = parentGroupWriter.createDatasetWriter(getName());
  return writeH5ObjectAttributes(dataStructureWriter, datasetWriter);
}

#if !defined(__APPLE__) && !defined(_MSC_VER)
#undef COMPLEX_EXPORT
#define COMPLEX_EXPORT
#endif

template class COMPLEX_EXPORT NeighborList<char>;

template class COMPLEX_EXPORT NeighborList<int8>;
template class COMPLEX_EXPORT NeighborList<uint8>;

template class COMPLEX_EXPORT NeighborList<int16>;
template class COMPLEX_EXPORT NeighborList<uint16>;

template class COMPLEX_EXPORT NeighborList<int32>;
template class COMPLEX_EXPORT NeighborList<uint32>;

template class COMPLEX_EXPORT NeighborList<int64>;
template class COMPLEX_EXPORT NeighborList<uint64>;

template class COMPLEX_EXPORT NeighborList<float32>;
template class COMPLEX_EXPORT NeighborList<float64>;

#if defined(__APPLE__) || defined(_MSC_VER)
template class COMPLEX_EXPORT NeighborList<usize>;
#endif