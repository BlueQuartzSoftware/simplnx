#include "NeighborList.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/ListStore.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/StoreCopyUtilities.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"

namespace nx::core
{
template <typename T>
NeighborList<T>::NeighborList(DataStructure& dataStructure, const std::string& name, const ShapeType& tupleShape)
: INeighborList(dataStructure, name)
, m_Store(std::make_shared<ListStore<T>>(tupleShape))
, m_IsAllocated(false)
, m_InitValue(static_cast<T>(0.0))
{
}

template <typename T>
NeighborList<T>::NeighborList(DataStructure& dataStructure, const std::string& name, const std::vector<SharedVectorType>& dataVector, IdType importId)
: INeighborList(dataStructure, name, importId)
, m_Store(std::make_shared<ListStore<T>>(dataVector))
, m_IsAllocated(true)
, m_InitValue(static_cast<T>(0.0))
{
}

template <typename T>
NeighborList<T>::NeighborList(DataStructure& dataStructure, const std::string& name, const std::shared_ptr<store_type>& dataStore, IdType importId)
: INeighborList(dataStructure, name, importId)
, m_Store(dataStore)
, m_IsAllocated(true)
, m_InitValue(static_cast<T>(0.0))
{
}

template <typename T>
NeighborList<T>::NeighborList(DataStructure& dataStructure, const std::string& name, const std::shared_ptr<store_type>& dataStore)
: INeighborList(dataStructure, name)
, m_Store(dataStore)
, m_IsAllocated(true)
, m_InitValue(static_cast<T>(0.0))
{
}

template <typename T>
NeighborList<T>* NeighborList<T>::Create(DataStructure& dataStructure, const std::string& name, const ShapeType& tupleShape, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<NeighborList>(new NeighborList(dataStructure, name, tupleShape));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

template <typename T>
NeighborList<T>* NeighborList<T>::Create(DataStructure& dataStructure, const std::string& name, const std::shared_ptr<store_type>& listStore, const std::optional<IdType>& parentId)
{
  if(listStore == nullptr)
  {
    return nullptr;
  }

  auto data = std::shared_ptr<NeighborList>(new NeighborList(dataStructure, name, listStore));
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
NeighborList<T>* NeighborList<T>::Import(DataStructure& dataStructure, const std::string& name, IdType importId, const std::shared_ptr<store_type>& dataStore, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<NeighborList>(new NeighborList(dataStructure, name, dataStore, importId));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

template <typename T>
NeighborList<T>::NeighborList(const NeighborList<T>& other)
: INeighborList(other)
, m_Store(other.m_Store)
, m_IsAllocated(other.m_IsAllocated)
, m_InitValue(other.m_InitValue)
{
}

template <typename T>
NeighborList<T>& NeighborList<T>::operator=(const NeighborList<T>& rhs)
{
  if(this == &rhs)
  {
    return *this;
  }

  const auto* destination = getDataStructure();
  const auto paths = getDataPaths();
  if(destination == nullptr || paths.empty() || destination->getData(paths.front()) != this)
  {
    throw std::runtime_error("NeighborList assignment to '" + getName() + "' requires an available destination array path");
  }
  // Tuple count supplies the existing lower-bound estimate without reading any list values.
  const auto bytes = CalculateStoreCopyBytes(rhs.getTupleShape(), ShapeType{1}, sizeof(T));
  const auto format = ArrayCreationUtilities::ResolveStorageFormat(*destination, paths.front(), GetDataType<T>(), bytes, "");
  m_Store = rhs.m_Store->deepCopy(format);
  m_IsAllocated = rhs.m_IsAllocated;
  m_InitValue = rhs.m_InitValue;

  return *this;
}

template <typename T>
NeighborList<T>& NeighborList<T>::operator=(NeighborList<T>&& rhs) noexcept
{
  m_Store = std::move(rhs.m_Store);
  m_IsAllocated = rhs.m_IsAllocated; // trivially copyable, move does nothing
  m_InitValue = std::move(rhs.m_InitValue);

  return *this;
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
  // This lower-bound estimate avoids reading lists merely to select destination storage.
  const auto bytes = CalculateStoreCopyBytes(getTupleShape(), ShapeType{1}, sizeof(T));
  const auto format = ArrayCreationUtilities::ResolveStorageFormat(dataStruct, copyPath, GetDataType<T>(), bytes, "");
  auto copiedStore = m_Store->deepCopy(format);
  // Insertion assigns the identifier for the copy.
  auto copy = std::shared_ptr<NeighborList<T>>(new NeighborList<T>(dataStruct, copyPath.getTargetName(), std::shared_ptr<store_type>(std::move(copiedStore))));
  copy->setNumNeighborsArrayName(getNumNeighborsArrayName());
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

  auto indicesSize = static_cast<usize>(idxs.size());
  if(indicesSize >= getNumberOfTuples())
  {
    Result<> resizeResult = resizeTuples(ShapeType{0});
    if(resizeResult.invalid())
    {
      return resizeResult.errors()[0].code;
    }
    return 0;
  }

  usize arraySize = m_Store->size();
  // Reject an index outside the list before the copy changes any values.
  for(usize idx : idxs)
  {
    if(idx >= arraySize)
    {
      return -100;
    }
  }

  const auto* destination = getDataStructure();
  const auto paths = getDataPaths();
  if(destination == nullptr || paths.empty() || destination->getData(paths.front()) != this)
  {
    return k_MissingCopyDestinationError;
  }
  // Parent order selects the first available path. The estimate does not read list values.
  const auto bytes = CalculateStoreCopyBytes(getTupleShape(), ShapeType{1}, sizeof(T));
  const auto format = ArrayCreationUtilities::ResolveStorageFormat(*destination, paths.front(), GetDataType<T>(), bytes, "");
  auto copy = m_Store->deepCopy(format);
  Result<> resizeResult = copy->resizeTuples(ShapeType{static_cast<ShapeType::value_type>(arraySize - indicesSize)});
  if(resizeResult.invalid())
  {
    return resizeResult.errors()[0].code;
  }

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
      if(idxsIndex == indicesSize)
      {
        idxsIndex--;
      }
    }
  }
  m_Store = std::move(copy);
  return err;
}

template <typename T>
void NeighborList<T>::copyTuple(usize currentPos, usize newPos)
{
  m_Store->setList(newPos, m_Store->at(currentPos));
}

template <typename T>
void NeighborList<T>::swapTuples(usize index0, usize index1)
{
  if(index0 == index1)
  {
    return;
  }
  auto value0 = m_Store->getList(index0);
  m_Store->setList(index0, m_Store->getList(index1));
  m_Store->setList(index1, value0);
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
void NeighborList<T>::addEntry(int32 grainId, value_type value)
{
  if(grainId >= static_cast<int32>(m_Store->size()))
  {
    usize old = m_Store->size();
    Result<> resizeResult = m_Store->resizeTuples(ShapeType{static_cast<ShapeType::value_type>(grainId + 1)});
    if(resizeResult.invalid())
    {
      throw std::runtime_error(resizeResult.errors()[0].message);
    }
    m_IsAllocated = true;
    // Initialize with zero length Vectors
    for(usize i = old; i < m_Store->size(); ++i)
    {
      m_Store->setList(i, SharedVectorType(new VectorType));
    }
  }
  m_Store->addEntry(grainId, value);
}

template <typename T>
void NeighborList<T>::updateListEntry(int32 grainId, usize elementPosition, value_type value)
{
  // The store does bound checking
  m_Store->setValue(grainId, elementPosition, value);
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
    Result<> resizeResult = m_Store->resizeTuples(ShapeType{static_cast<ShapeType::value_type>(grainId + 1)});
    if(resizeResult.invalid())
    {
      throw std::runtime_error(resizeResult.errors()[0].message);
    }
    m_IsAllocated = true;
  }
  m_Store->setList(grainId, neighborList);
}

template <typename T>
void NeighborList<T>::setList(int32 grainId, const VectorType& neighborList)
{
  if(grainId >= static_cast<int32>(m_Store->size()))
  {
    Result<> resizeResult = m_Store->resizeTuples(ShapeType{static_cast<ShapeType::value_type>(grainId + 1)});
    if(resizeResult.invalid())
    {
      throw std::runtime_error(resizeResult.errors()[0].message);
    }
    m_IsAllocated = true;
  }
  m_Store->setList(grainId, neighborList);
}

template <typename T>
void NeighborList<T>::setLists(const std::vector<std::vector<T>>& neighborLists)
{
  m_Store->setData(neighborLists);
}

template <typename T>
T NeighborList<T>::getValue(int32 grainId, int32 index, bool& ok) const
{
  return m_Store->getValue(grainId, index, ok);
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
IListStore* NeighborList<T>::getIListStore()
{
  return m_Store.get();
}

template <typename T>
const IListStore* NeighborList<T>::getIListStore() const
{
  return m_Store.get();
}

template <typename T>
std::shared_ptr<typename NeighborList<T>::store_type> NeighborList<T>::getStore() const
{
  return m_Store;
}

template <typename T>
void NeighborList<T>::setStore(const std::shared_ptr<store_type>& store)
{
  m_Store = store;
}

template <typename T>
std::vector<typename NeighborList<T>::VectorType> NeighborList<T>::getVectors() const
{
  usize count = m_Store->size();
  std::vector<typename NeighborList<T>::VectorType> vectors(count);
  for(usize i = 0; i < count; i++)
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
  return m_Store->cbegin();
}

template <typename T>
typename NeighborList<T>::const_iterator NeighborList<T>::end() const
{
  return m_Store->cend();
}

template <typename T>
typename NeighborList<T>::const_iterator NeighborList<T>::cbegin() const
{
  return m_Store->cbegin();
}

template <typename T>
typename NeighborList<T>::const_iterator NeighborList<T>::cend() const
{
  return m_Store->cend();
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

template <class T>
std::string NeighborList<T>::toString(usize tupleIndex, usize compIndex, const std::string& format) const
{
  bool hasValueAtCell = false;
  const T value = getValue(tupleIndex, compIndex, hasValueAtCell);
  if(!hasValueAtCell)
  {
    return "";
  }
  if constexpr(std::is_floating_point_v<T>)
  {
    return fmt::format(fmt::runtime(format), value);
  }
  else
  {
    return fmt::format("{}", value);
  }
}

template <class T>
bool NeighborList<T>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value)
{
  Result<T> result = nx::core::StringInterpretationUtilities::Convert<T>(value);
  if(result.invalid())
  {
    return false;
  }
  updateListEntry(tupleIndex, compIndex, result.value());
  return true;
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
