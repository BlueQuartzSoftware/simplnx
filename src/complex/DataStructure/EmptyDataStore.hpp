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

  ~EmptyDataStore() override = default;

  /**
   * @brief Returns the number of tuples that should be in the data store.
   * @return usize
   */
  usize getNumberOfTuples() const override
  {
    return 0;
  }

  /**
   * @brief Returns the target tuple getSize.
   * @return usize
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
  value_type getValue(usize index) const override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target getSize.
   * @param index
   * @param value
   */
  void setValue(usize index, value_type value) override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target getSize.
   * @param index
   * @return const_reference
   */
  typename IDataStore<T>::const_reference at(usize index) const override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target getSize.
   * @param  index
   * @return const_reference
   */
  typename IDataStore<T>::const_reference operator[](usize index) const override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target getSize.
   * @param  index
   * @return reference
   */
  typename IDataStore<T>::reference operator[](usize index) override
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
   * @brief Throws a runtime error due to the inability to write values to HDF5.
   * @param datasetWriter
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DatasetWriter& datasetWriter) const override
  {
    throw std::runtime_error("");
  }

private:
  ShapeType m_ComponentShape;
  ShapeType m_TupleShape;
};
} // namespace complex
