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
   * @brief
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
   * @brief
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

  DataArray(const DataArray<T>& other)
  : DataObject(other)
  , m_TupleCount(other.m_TupleCount)
  , m_TupleSize(other.m_TupleSize)
  , m_DataStore(other.m_DataStore)
  {
  }

  DataArray(DataArray<T>&& other) noexcept
  : DataObject(other)
  , m_TupleCount(std::move(other.m_TupleCount))
  , m_TupleSize(std::move(other.m_TupleSize))
  , m_DataStore(std::move(other.m_DataStore))
  {
  }

  /**
   * Empty Destructor
   */
  virtual ~DataArray() = default;

  /**
   * @brief Returns the number of elements in the data array.
   * @return size_t
   */
  size_t size() const;

  /**
   * @brief Returns the number of tuples in the DataArray.
   * @return size_t
   */
  size_t getTupleCount() const;

  /**
   * @brief Returns the tuple size.
   * @return size_t
   */
  size_t getTupleSize() const;

  /**
   * @brief Returns the value found at the specified index of the data array.
   * Throws an exception if the DataStore has not been allocated. This can be
   * used to edit the value found at the specified index.
   * @param  index
   */
  T& operator[](size_t index);

  /**
   * @brief Returns the value found at the specified index of the data array.
   * Throws an exception if the DataStore has not been allocated. This cannot be
   * used to edit the value found at the specified index.
   * @param  index
   */
  T operator[](size_t index) const;

  /**
   * @brief Returns an Iterator to the begining of the data array.
   * @return Iterator
   */
  Iterator begin();

  /**
   * @brief Returns an Iterator to the end of the data array.
   * @return Iterator
   */
  Iterator end();

  /**
   * @brief Returns an ConstIterator to the begining of the data array.
   * @return ConstIterator
   */
  ConstIterator begin() const;

  /**
   * @brief Returns an ConstIterator to the end of the data array.
   * @return ConstIterator
   */
  ConstIterator end() const;

  /**
   * @brief
   * @return DataStore<T>*
   */
  const DataStore<T>* getDataStore() const;

  /**
   * @brief
   * @return DataStore<T>*
   */
  DataStore<T>* getDataStore();

  /**
   * @brief
   * @return
   */
  bool isAllocated() const;

  /**
   * @brief Releases and returns the DataStore from memory.
   * @return DataStore<T>*
   */
  DataStore<T>* releaseDataStore();

  /**
   * @brief Sets a new DataStore for the DataArray to handle. The existing DataStore is deleted.
   * To save the existing DataStore before replacing it, call releaseDataStore() before setting the new DataStore.
   * @param store
   */
  void setDataStore(DataStore<T>* store);

  /**
   * @brief
   * @param out
   * @param hdfFileName
   * @return H5::ErrorType
   */
  H5::ErrorType generateXdmfText(std::ostream& out, const std::string& hdfFileName) const override;

  /**
   * @brief
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
