#pragma once

#include <stdexcept>

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"

namespace complex
{
/**
 * @class DataArray
 * @brief The DataArray class is a type of DataObject that exists to store and
 * retrieve array data within the DataStructure. The DataArray is designed to
 * allow expandability into multiple sources of data, including out-of-core data.
 */
template <class T>
class DataArray : public DataObject
{
public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using store_type = IDataStore<T>;
  using weak_store = typename std::weak_ptr<IDataStore<T>>;
  using Iterator = typename IDataStore<T>::Iterator;
  using ConstIterator = typename IDataStore<T>::ConstIterator;

  /**
   * @brief Constructs a DataArray with the specified tuple size, count, and
   * constructs a DataStore with the provided default value.
   * @param ds
   * @param name
   * @param store
   */
  DataArray(DataStructure* ds, const std::string& name, store_type* store = nullptr)
  : DataObject(ds, name)
  {
    setDataStore(store);
  }

  /**
   * @brief Constructs a DataArray with the specified tuple size, count, and
   * DataStore. The DataArray takes ownership of the DataStore.
   * @param ds
   * @param name
   * @param dataStore
   */
  DataArray(DataStructure* ds, const std::string& name, const weak_store& store)
  : DataObject(ds, name)
  {
    setDataStore(store);
  }

  /**
   * @brief Copy constructor creates a copy of the specified tuple size, count,
   * and smart pointer to the target DataStore.
   * @param other
   */
  DataArray(const DataArray<T>& other)
  : DataObject(other)
  , m_DataStore(other.m_DataStore)
  {
  }

  /**
   * @brief Move constructor moves the data from the provided DataArray.
   * @param other
   */
  DataArray(DataArray<T>&& other) noexcept
  : DataObject(std::move(other))
  , m_DataStore(std::move(other.m_DataStore))
  {
  }

  virtual ~DataArray() = default;

  /**
   * @brief Returns a shallow copy of the DataArray without copying data
   * store's contents.
   * @return DataObject*
   */
  DataObject* shallowCopy() override
  {
    return new DataArray(*this);
  }

  /**
   * @brief Returns a deep copy of the DataArray including a deep copy of the
   * data store.
   * @return DataObject*
   */
  DataObject* deepCopy() override
  {
    return new DataArray(getDataStructure(), getName(), getDataStore()->deepCopy());
  }

  /**
   * @brief Returns the number of elements in the data array.
   * @return size_t
   */
  size_t getSize() const
  {
    return getTupleCount() * getTupleSize();
  }

  /**
   * @brief Returns the number of tuples in the DataArray.
   * @return size_t
   */
  size_t getTupleCount() const
  {
    return m_DataStore->getTupleCount();
  }

  /**
   * @brief Returns the tuple size.
   * @return size_t
   */
  size_t getTupleSize() const
  {
    return m_DataStore->getTupleSize();
  }

  /**
   * @brief Returns the value found at the specified index of the data array.
   * Throws an exception if the DataStore has not been allocated. This can be
   * used to edit the value found at the specified index.
   * @param  index
   * @return reference
   */
  reference operator[](size_t index)
  {
    if(m_DataStore == nullptr)
    {
      throw std::runtime_error("");
    }

    return (*m_DataStore.get())[index];
  }

  /**
   * @brief Returns the value found at the specified index of the data array.
   * Throws an exception if the DataStore has not been allocated. This cannot be
   * used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  const_reference operator[](size_t index) const
  {
    if(m_DataStore == nullptr)
    {
      throw std::runtime_error("");
    }

    return (*m_DataStore.get())[index];
  }

  /**
   * @brief Returns the value found at the specified index of the data array.
   * Throws an exception if the DataStore has not been allocated. This cannot be
   * used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  const_reference at(size_t index) const
  {
    if(m_DataStore == nullptr)
    {
      throw std::runtime_error("");
    }

    return (*m_DataStore.get())[index];
  }

  /**
   * @brief Returns a raw pointer to the DataStore for read-only access.
   * @return DataStore<T>*
   */
  const store_type* getDataStore() const
  {
    return m_DataStore.get();
  }

  /**
   * @brief Returns a raw pointer to the DataStore.
   * @return DataStore<T>*
   */
  store_type* getDataStore()
  {
    return m_DataStore.get();
  }

  /**
   * @brief Returns a std::weak_ptr for the stored DataStore.
   * @return std::weak_ptr<DataStore<T>>
   */
  weak_store getDataStorePtr()
  {
    return m_DataStore;
  }

  /**
   * @brief Returns true if the DataStore has already been allocated. Returns false otherwise.
   * @return bool
   */
  bool isAllocated() const
  {
    return m_DataStore != nullptr;
  }

  /**
   * @brief Sets a new DataStore for the DataArray to handle. The existing DataStore
   * is deleted if there are no other references. To save the existing DataStore
   * before replacing it, call releaseDataStore() before setting the new DataStore.
   * @param store
   */
  void setDataStore(store_type* store)
  {
    m_DataStore = std::shared_ptr<store_type>(store);
    if(m_DataStore == nullptr)
    {
      m_DataStore = std::shared_ptr<store_type>(new EmptyDataStore<T>());
    }
  }

  /**
   * @brief Sets a new DataStore for the DataArray to handle. The existing DataStore
   * is deleted if there are no other references. To save the existing DataStore
   * before replacing it, call releaseDataStore() before setting the new DataStore.
   * @param store
   */
  void setDataStore(const weak_store& store)
  {
    m_DataStore = store.lock();
    if(m_DataStore == nullptr)
    {
      m_DataStore = std::shared_ptr<store_type>(new EmptyDataStore<T>());
    }
  }

  /**
   * @brief Returns the first item in the array.
   * @return value_type
   */
  value_type front() const
  {
    return at(0);
  }

  /**
   * @brief Returns the last item in the array.
   * @return value_type
   */
  value_type back() const
  {
    return at(getSize() - 1);
  }

  /**
   * @brief Returns an Iterator to the begining of the data array.
   * @return Iterator
   */
  Iterator begin()
  {
    return getDataStore()->begin();
  }

  /**
   * @brief Returns an Iterator to the end of the data array.
   * @return Iterator
   */
  Iterator end()
  {
    return getDataStore()->end();
  }

  /**
   * @brief Returns an ConstIterator to the begining of the data array.
   * @return ConstIterator
   */
  ConstIterator begin() const
  {
    return getDataStore()->begin();
  }

  /**
   * @brief Returns an ConstIterator to the end of the data array.
   * @return ConstIterator
   */
  ConstIterator end() const
  {
    return getDataStore()->end();
  }

  /**
   * @brief Generates text for the XDMF file and appends it to the output stream.
   * Returns an H5::ErrorType specifying any error that might have occurred.
   * @param out
   * @param hdfFileName
   * @return H5::ErrorType
   */
  H5::ErrorType generateXdmfText(std::ostream& out, const std::string& hdfFileName) const override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Reads and overwrites data from the provided input stream. Returns an
   * H5::ErrorType specifying any error that might have occurred.
   * @param in
   * @param hdfFileName
   * @return H5::ErrorType
   */
  H5::ErrorType readFromXdmfText(std::istream& in, const std::string& hdfFileName) override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Copy assignment operator
   * @param rhs
   * @return DataArray&
   */
  DataArray& operator=(const DataArray& rhs)
  {
    m_DataStore = rhs.m_DataStore;
    return *this;
  }

  /**
   * @brief Move assignment operator
   * @param rhs
   * @return DataArray&
   */
  DataArray& operator=(DataArray&& rhs)
  {
    m_DataStore = std::move(rhs.m_DataStore);
    return *this;
  }

protected:
private:
  std::shared_ptr<IDataStore<T>> m_DataStore = nullptr;
};

// Declare extern templates
// extern template class DataArray<uint8_t>;
// extern template class DataArray<uint16_t>;
// extern template class DataArray<uint32_t>;
// extern template class DataArray<uint64_t>;

// extern template class DataArray<int8_t>;
// extern template class DataArray<int16_t>;
// extern template class DataArray<int32_t>;
// extern template class DataArray<int64_t>;
// extern template class DataArray<size_t>;

// extern template class DataArray<float>;
// extern template class DataArray<double>;

// Declare aliases
using CharArray = DataArray<char>;
using UCharArray = DataArray<unsigned char>;

using UInt8Array = DataArray<uint8_t>;
using UInt16Array = DataArray<uint16_t>;
using UInt32Array = DataArray<uint32_t>;
using UInt64Array = DataArray<uint64_t>;

using Int8Array = DataArray<int8_t>;
using Int16Array = DataArray<int16_t>;
using Int32Array = DataArray<int32_t>;
using Int64Array = DataArray<int64_t>;
using SizeArray = DataArray<size_t>;

using FloatArray = DataArray<float>;
using DoubleArray = DataArray<double>;
using VectorOfFloatArray = std::vector<std::shared_ptr<FloatArray>>;

} // namespace complex
