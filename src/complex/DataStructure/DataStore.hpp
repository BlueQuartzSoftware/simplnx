#pragma once

#include "complex/DataStructure/IDataStore.hpp"

namespace complex
{
/**
 * @class DataStore
 * @brief The DataStore class handles the storing and retrieval of data for
 * use in DataArrays.
 * @tparam T
 */
template <typename T>
class DataStore : public IDataStore<T>
{
public:
  /**
   * @brief Constructs a DataStore with the specified tupleSize and tupleCount.
   * @param tupleSize
   * @param tupleCount
   */
  DataStore(size_t tupleSize, size_t tupleCount)
  : m_TupleSize(tupleSize)
  , m_TupleCount(tupleCount)
  , m_Data(tupleSize * tupleCount)
  {
  }

  /**
   * @brief Copy constructor
   * @param other
   */
  DataStore(const DataStore& other)
  : m_TupleCount(other.m_TupleCount)
  , m_TupleSize(other.m_TupleSize)
  , m_Data(other.m_Data)
  {
  }

  /**
   * @brief Move constructor
   * @param other
   */
  DataStore(DataStore&& other) noexcept
  : m_TupleCount(std::move(other.m_TupleCount))
  , m_TupleSize(std::move(other.m_TupleSize))
  , m_Data(std::move(other.m_Data))
  {
  }

  virtual ~DataStore() = default;

  /**
   * @brief Returns the number of tuples in the DataStore.
   * @return size_t
   */
  size_t getTupleCount() const override
  {
    return m_TupleCount;
  }

  /**
   * @brief Returns the tuple size.
   * @return size_t
   */
  size_t getTupleSize() const override
  {
    return m_TupleSize;
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return value_type
   */
  value_type getValue(size_t index) const override
  {
    return m_Data[index];
  }

  /**
   * @brief Sets the value stored at the specified index.
   * @param index
   * @param value
   */
  void setValue(size_t index, value_type value) override
  {
    m_Data[index] = value;
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param  index
   * @return const_reference
   */
  const_reference operator[](size_t index) const override
  {
    return m_Data[index];
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This can be used to edit the value found at the specified index.
   * @param  index
   * @return reference
   */
  reference operator[](size_t index) override
  {
    return m_Data[index];
  }

private:
  size_t m_TupleSize;
  size_t m_TupleCount;
  std::vector<value_type> m_Data;
};
} // namespace complex
