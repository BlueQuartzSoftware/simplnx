#pragma once

#include "complex/DataStructure/IDataStore.hpp"

#include <stdexcept>
#include <vector>

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
  using ShapeType = typename std::vector<size_t>;

  /**
   * @brief Constructs an empty data store with a tuple getSize and count of 0.
   */
  EmptyDataStore()
  {
  }

  /**
   * @brief Constructs an empty data store with the specified tupleSize and tupleCount.
   * @param tupleSize
   * @param tupleCount
   */
  EmptyDataStore(const ShapeType& tupleShape, const ShapeType& componentShape)
  : m_ComponentShape(componentShape)
  , m_TupleShape(tupleShape)
  {
  }

  /**
   * @brief Copy constructor
   * @param other
   */
  EmptyDataStore(const EmptyDataStore& other)
  : m_TupleShape(other.m_TupleShape)
  , m_ComponentShape(other.m_ComponentShape)
  {
  }

  /**
   * @brief Move constructor
   * @param other
   */
  EmptyDataStore(EmptyDataStore&& other) noexcept
  : m_TupleShape(std::move(other.m_TupleShape))
  , m_ComponentShape(std::move(other.m_ComponentShape))
  {
  }

  virtual ~EmptyDataStore() = default;

  /**
   * @brief Returns the number of tuples in the DataStore.
   * @return size_t
   */
  size_t getNumberOfTuples() const override
  {
    return 0;
  }

  /**
   * @brief Returns the number of elements in each Tuple.
   * @return size_t
   */
  size_t getNumberOfComponents() const override
  {
    return 0;
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target size.
   * @param tupleShape
   */
  void reshapeTuples(const ShapeType& tupleShape) override
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
<<<<<<< HEAD
  size_t m_NumComponents;
  size_t m_TupleCount;
=======
  ShapeType m_ComponentShape;
  ShapeType m_TupleShape;
>>>>>>> 64ab861 (refactor DatArray and DataStore to use std::vector<size_t> for Tuple and Comp dimensions)
};
} // namespace complex
