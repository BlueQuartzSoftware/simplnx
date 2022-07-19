#include "NeighborList.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DatasetReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupWriter.hpp"

namespace complex
{
template <typename T>
NeighborList<T>::NeighborList(DataStructure& dataStructure, const std::string& name, usize numTuples)
: INeighborList(dataStructure, name, numTuples)
, m_IsAllocated(false)
, m_InitValue(static_cast<T>(0.0))
{
}

template <typename T>
NeighborList<T>::NeighborList(DataStructure& dataStructure, const std::string& name, const std::vector<SharedVectorType>& dataVector, IdType importId)
: INeighborList(dataStructure, name, dataVector.size(), importId)
, m_Array(dataVector)
, m_IsAllocated(true)
, m_InitValue(static_cast<T>(0.0))
{
}

template <typename T>
NeighborList<T>* NeighborList<T>::Create(DataStructure& ds, const std::string& name, usize numTuples, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<NeighborList>(new NeighborList(ds, name, numTuples));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

template <typename T>
NeighborList<T>* NeighborList<T>::Import(DataStructure& ds, const std::string& name, IdType importId, const std::vector<SharedVectorType>& dataVector, const std::optional<IdType>& parentId)
{

  auto data = std::shared_ptr<NeighborList>(new NeighborList(ds, name, dataVector, importId));
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
  copy->setNumNeighborsArrayName(getNumNeighborsArrayName());
  copy->m_Array = m_Array;
  return copy;
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
  for(unsigned long idx : idxs)
  {
    if(idx >= arraySize)
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
  setNumberOfTuples(m_Array.size());
  return err;
}

template <typename T>
void NeighborList<T>::copyTuple(usize currentPos, usize newPos)
{
  m_Array[newPos] = m_Array[currentPos];
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
  setNumberOfTuples(size);
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
  if(grainId >= static_cast<int32>(m_Array.size()))
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
  setNumberOfTuples(m_Array.size());
}

template <typename T>
void NeighborList<T>::clearAllLists()
{
  m_Array.clear();
  m_IsAllocated = false;
}

template <typename T>
void NeighborList<T>::setList(int32 grainId, const SharedVectorType& neighborList)
{
  if(grainId >= static_cast<int32>(m_Array.size()))
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
    return static_cast<T>(-1);
  }
  return (*vec)[index];
}

template <typename T>
int32 NeighborList<T>::getNumberOfLists() const
{
  return static_cast<int32>(m_Array.size());
}

template <typename T>
int32 NeighborList<T>::getListSize(int32 grainId) const
{
  return static_cast<int32>(m_Array[grainId]->size());
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

template <>
DataType COMPLEX_EXPORT NeighborList<int8>::getDataType() const
{
  return DataType::int8;
}

template <>
DataType COMPLEX_EXPORT NeighborList<int16>::getDataType() const
{
  return DataType::int16;
}

template <>
DataType COMPLEX_EXPORT NeighborList<int32>::getDataType() const
{
  return DataType::int32;
}

template <>
DataType COMPLEX_EXPORT NeighborList<int64>::getDataType() const
{
  return DataType::int64;
}

template <>
DataType COMPLEX_EXPORT NeighborList<uint8>::getDataType() const
{
  return DataType::uint8;
}

template <>
DataType COMPLEX_EXPORT NeighborList<uint16>::getDataType() const
{
  return DataType::uint16;
}

template <>
DataType COMPLEX_EXPORT NeighborList<uint32>::getDataType() const
{
  return DataType::uint32;
}

template <>
DataType COMPLEX_EXPORT NeighborList<uint64>::getDataType() const
{
  return DataType::uint64;
}

#if defined(__APPLE__)
template <>
DataType COMPLEX_EXPORT NeighborList<unsigned long>::getDataType() const
{
  return DataType::uint64;
}
#endif

template <>
DataType COMPLEX_EXPORT NeighborList<float32>::getDataType() const
{
  return DataType::float32;
}

template <>
DataType COMPLEX_EXPORT NeighborList<float64>::getDataType() const
{
  return DataType::float64;
}

template <typename T>
H5::ErrorType NeighborList<T>::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  DataStructure tmp;

  // Create NumNeighbors DataStore
  const usize arraySize = m_Array.size();
  auto* numNeighborsArray = Int32Array::CreateWithStore<Int32DataStore>(tmp, getNumNeighborsArrayName(), {arraySize}, {1});
  auto& numNeighborsStore = numNeighborsArray->getDataStoreRef();
  usize totalItems = 0;
  for(usize i = 0; i < arraySize; i++)
  {
    const auto numNeighbors = m_Array[i]->size();
    numNeighborsStore[i] = static_cast<int32>(numNeighbors);
    totalItems += numNeighbors;
  }

  // Write NumNeighbors data
  auto error = numNeighborsArray->writeHdf5(dataStructureWriter, parentGroupWriter, false);
  if(error < 0)
  {
    return error;
  }

  // Create flattened neighbor DataStore
  DataStore<T> flattenedData(totalItems, static_cast<T>(0));
  usize offset = 0;
  for(const auto& segment : m_Array)
  {
    usize numElements = segment->size();
    if(numElements == 0)
    {
      continue;
    }
    T* start = segment->data();
    for(usize i = 0; i < numElements; i++)
    {
      flattenedData[offset + i] = start[i];
    }
    offset += numElements;
  }

  // Write flattened array to HDF5 as a separate array
  auto datasetWriter = parentGroupWriter.createDatasetWriter(getName());
  H5::ErrorType err = flattenedData.writeHdf5(datasetWriter);
  if(err < 0)
  {
    return err;
  }
  auto linkedDatasetAttribute = datasetWriter.createAttribute("Linked NumNeighbors Dataset");
  err = linkedDatasetAttribute.writeString(getNumNeighborsArrayName());
  if(err < 0)
  {
    return err;
  }
  return writeH5ObjectAttributes(dataStructureWriter, datasetWriter, importable);
}

template <typename T>
std::vector<typename NeighborList<T>::SharedVectorType> NeighborList<T>::ReadHdf5Data(const H5::GroupReader& parentGroup, const H5::DatasetReader& dataReader)
{
  auto numNeighborsAttributeName = dataReader.getAttribute("Linked NumNeighbors Dataset");
  auto numNeighborsName = numNeighborsAttributeName.readAsString();

  auto numNeighborsReader = parentGroup.openDataset(numNeighborsName);

  auto numNeighborsPtr = Int32DataStore::ReadHdf5(numNeighborsReader);
  auto& numNeighborsStore = *numNeighborsPtr.get();

  auto flatDataStorePtr = DataStore<T>::ReadHdf5(dataReader);
  auto& flatDataStore = *flatDataStorePtr.get();

  std::vector<SharedVectorType> dataVector;
  usize offset = 0;
  const auto numTuples = numNeighborsStore.getNumberOfTuples();
  for(usize i = 0; i < numTuples; i++)
  {
    const auto numNeighbors = numNeighborsStore[i];
    auto sharedVector = std::make_shared<std::vector<T>>(numNeighbors);
    std::vector<T>& vector = *sharedVector.get();

    for(usize j = 0; j < numNeighbors; j++)
    {
      vector[j] = flatDataStore[offset + j];
    }
    offset += numNeighbors;
    dataVector.push_back(sharedVector);
  }

  return dataVector;
}

#if !defined(__APPLE__) && !defined(_MSC_VER)
#undef COMPLEX_EXPORT
#define COMPLEX_EXPORT
#endif

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
} // namespace complex
