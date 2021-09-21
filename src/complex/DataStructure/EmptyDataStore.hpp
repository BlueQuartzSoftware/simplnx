#pragma once

#include <stdexcept>

#include "complex/DataStructure/IDataStore.hpp"

namespace complex
{
/**
 * @class EmptyDataStore
 * @brief The EmptyDataStore class serves as a placeholder IDataStore for use
 * in preflight where data is not available but the number and getSize of tuples
 * is known.
 * @tparam T
 */
template <typename T>
class EmptyDataStore : public IDataStore<T>
{
public:
  using value_type = typename IDataStore<T>::value_type;
  using reference = typename IDataStore<T>::reference;
  using const_reference = typename IDataStore<T>::const_reference;

  /**
   * @brief Constructs an empty data store with a tuple getSize and count of 0.
   */
  EmptyDataStore()
  : m_TupleSize(0)
  , m_TupleCount(0)
  {
  }

  /**
   * @brief Constructs an empty data store with the specified tupleSize and tupleCount.
   * @param tupleSize
   * @param tupleCount
   */
  EmptyDataStore(size_t tupleSize, size_t tupleCount)
  : m_TupleSize(tupleSize)
  , m_TupleCount(tupleCount)
  {
  }

  /**
   * @brief Copy constructor
   * @param other
   */
  EmptyDataStore(const EmptyDataStore& other)
  : m_TupleCount(other.m_TupleCount)
  , m_TupleSize(other.m_TupleSize)
  {
  }

  /**
   * @brief Move constructor
   * @param other
   */
  EmptyDataStore(EmptyDataStore&& other) noexcept
  : m_TupleCount(std::move(other.m_TupleCount))
  , m_TupleSize(std::move(other.m_TupleSize))
  {
  }

  virtual ~EmptyDataStore() = default;

  /**
   * @brief Returns the number of tuples that should be in the data store.
   * @return size_t
   */
  size_t getTupleCount() const override
  {
    return m_TupleCount;
  }

  /**
   * @brief Returns the target tuple getSize.
   * @return size_t
   */
  size_t getTupleSize() const override
  {
    return m_TupleSize;
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target getSize.
   * @param tupleCount
   */
  void resizeTuples(size_t tupleCount) override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target getSize.
   * @param index
   * @return value_type
   */
  value_type getValue(size_t index) const override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target getSize.
   * @param index
   * @param value
   */
  void setValue(size_t index, value_type value) override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target getSize.
   * @param index
   * @return const_reference
   */
  typename IDataStore<T>::const_reference at(size_t index) const override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target getSize.
   * @param  index
   * @return const_reference
   */
  typename IDataStore<T>::const_reference operator[](size_t index) const override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target getSize.
   * @param  index
   * @return reference
   */
  typename IDataStore<T>::reference operator[](size_t index) override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Returns a deep copy of the data store and all its data.
   * @return IDataStore*
   */
  IDataStore<T>* deepCopy() const override
  {
    return new EmptyDataStore(*this);
  }

  /**
   * @brief Writes the data store to HDF5. Returns the HDF5 error code should
   * one be encountered. Otherwise, returns 0.
   * @param dataId
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::IdType dataId) const override
  {
    throw std::runtime_error("");
  }

private:
  size_t m_TupleSize;
  size_t m_TupleCount;
};
} // namespace complex
