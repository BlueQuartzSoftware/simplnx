#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStore.hpp"

namespace complex
{

/**
 * class DataArray
 *
 */

template <class T>
class DataArray : public DataObject
{
public:
  using Iterator = void;
  using ConstIterator = void;

  /**
   * @brief Constructs a DataArray with the specified tuple size, count, and constructs a DataStore with the provided default value.
   * @param ds
   * @param name
   * @param tupleSize
   * @param tupleCount
   * @param defaultValue
   */
  DataArray(DataStructure* ds, const std::string& name, size_t tupleSize, size_t tupleCount, T defaultValue)
  : DataObject(ds, name)
  , m_TupleSize(tupleSize)
  , m_TupleCount(tupleCount)
  {
  }

  /**
   * @brief Constructs a DataArray with the specified tuple size, count, and DataStore. The DataArray takes ownership of the DataStore.
   * @param ds
   * @param name
   * @param tupleSize
   * @param tupleCount
   * @param dataStore
   */
  DataArray(DataStructure* ds, const std::string& name, size_t tupleSize, size_t tupleCount, DataStore<T>* dataStore = nullptr)
  : DataObject(ds, name)
  , m_TupleSize(tupleSize)
  , m_TupleCount(tupleCount)
  {
  }

  /**
   * @brief Copy constructor creates a copy of the specified tuple size, count, and smart pointer to the target DataStore.
   * @param other
   */
  DataArray(const DataArray<T>& other)
  : DataObject(other)
  , m_TupleCount(other.m_TupleCount)
  , m_TupleSize(other.m_TupleSize)
  , m_DataStore(other.m_DataStore)
  {
  }

  /**
   * @brief Move constructor moves the data from the provided DataArray.
   * @param other
   * @return
   */
  DataArray(DataArray<T>&& other) noexcept
  : DataObject(std::move(other))
  , m_TupleCount(std::move(other.m_TupleCount))
  , m_TupleSize(std::move(other.m_TupleSize))
  , m_DataStore(std::move(other.m_DataStore))
  {
  }

  virtual ~DataArray() = default;

  /**
   * @brief Returns the number of elements in the data array.
   * @return size_t
   */
  size_t size() const
  {
    return getTupleCount() * getTupleSize();
  }

  /**
   * @brief Returns the number of tuples in the DataArray.
   * @return size_t
   */
  size_t getTupleCount() const
  {
    return m_TupleCount;
  }

  /**
   * @brief Returns the tuple size.
   * @return size_t
   */
  size_t getTupleSize() const
  {
    return m_TupleSize;
  }

  /**
   * @brief Returns the value found at the specified index of the data array.
   * Throws an exception if the DataStore has not been allocated. This can be
   * used to edit the value found at the specified index.
   * @param  index
   */
  T& operator[](size_t index)
  {
    if(nullptr == m_DataStore)
    {
      throw std::exception();
    }

    return (*m_DataStore.get())[index];
  }

  /**
   * @brief Returns the value found at the specified index of the data array.
   * Throws an exception if the DataStore has not been allocated. This cannot be
   * used to edit the value found at the specified index.
   * @param  index
   */
  T operator[](size_t index) const
  {
    if(nullptr == m_DataStore)
    {
      throw std::exception();
    }

    return (*m_DataStore.get())[index];
  }

  /**
   * @brief Returns an Iterator to the begining of the data array.
   * @return Iterator
   */
  Iterator begin()
  {
    return m_DataStore->begin();
  }

  /**
   * @brief Returns an Iterator to the end of the data array.
   * @return Iterator
   */
  Iterator end()
  {
    return m_DataStore->end();
  }

  /**
   * @brief Returns an ConstIterator to the begining of the data array.
   * @return ConstIterator
   */
  ConstIterator begin() const
  {
    return m_DataStore->begin();
  }

  /**
   * @brief Returns an ConstIterator to the end of the data array.
   * @return ConstIterator
   */
  ConstIterator end() const
  {
    return m_DataStore->end();
  }

  /**
   * @brief Returns a raw pointer to the DataStore for read-only access.
   * @return DataStore<T>*
   */
  const DataStore<T>* getDataStore() const
  {
    return m_DataStore.get();
  }

  /**
   * @brief Returns a raw pointer to the DataStore.
   * @return DataStore<T>*
   */
  DataStore<T>* getDataStore()
  {
    return m_DataStore.get();
  }

  /**
   * @brief Returns a std::weak_ptr for the stored DataStore.
   * @return std::weak_ptr<DataStore<T>>
   */
  std::weak_ptr<DataStore<T>> getDataStorePtr()
  {
    return m_DataStore;
  }

  /**
   * @brief Returns true if the DataStore has already been allocated. Returns false otherwise.
   * @return bool
   */
  bool isAllocated() const
  {
    return nullptr != m_DataStore;
  }

  /**
   * @brief Sets a new DataStore for the DataArray to handle. The existing DataStore
   * is deleted if there are no other references. To save the existing DataStore
   * before replacing it, call releaseDataStore() before setting the new DataStore.
   * @param store
   */
  void setDataStore(DataStore<T>* store)
  {
    m_DataStore = std::shared_ptr<DataStore<T>>(store);
  }

  /**
   * @brief Sets a new DataStore for the DataArray to handle. The existing DataStore
   * is deleted if there are no other references. To save the existing DataStore
   * before replacing it, call releaseDataStore() before setting the new DataStore.
   * @param store
   */
  void setDataStore(const std::weak_ptr<DataStore<T>>& store)
  {
    m_DataStore = store.lock();
  }

  /**
   * @brief Generates text for the XDMF file and appends it to the output stream.
   * Returns an H5::ErrorType specifying any error that might have occurred.
   * @param out
   * @param hdfFileName
   * @return H5::ErrorType
   */
  H5::ErrorType generateXdmfText(std::istream& out, const std::string& hdfFileName) const override;

  /**
   * @brief Reads and overwrites data from the provided input stream. Returns an
   * H5::ErrorType specifying any error that might have occurred.
   * @param in
   * @param hdfFileName
   * @return H5::ErrorType
   */
  H5::ErrorType readFromXdmfText(std::istream& in, const std::string& hdfFileName) override;

protected:
private:
  size_t m_TupleSize = 0;
  size_t m_TupleCount = 0;
  std::shared_ptr<DataStore<T>> m_DataStore = nullptr;
};

using FloatArray = DataArray<float>;
using DoubleArray = DataArray<double>;
using VectorOfFloatArray = std::vector<std::shared_ptr<FloatArray>>;

} // namespace complex
