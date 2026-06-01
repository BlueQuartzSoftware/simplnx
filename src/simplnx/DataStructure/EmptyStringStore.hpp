#pragma once

#include "AbstractStringStore.hpp"

#include <functional>
#include <numeric>
#include <stdexcept>

namespace nx::core
{
/**
 * @class EmptyStringStore
 * @brief A metadata-only placeholder for AbstractStringStore, analogous to
 * EmptyDataStore for numeric arrays.
 *
 * EmptyStringStore records tuple shape (number and layout of strings) but
 * holds no actual string data. It exists because:
 *
 * 1. **Preflight-style imports:** When loading a .dream3d file's
 *    DataStructure in metadata-only mode (e.g., for file inspection or
 *    pipeline validation), StringArray objects need a store that reports
 *    correct tuple counts without allocating or reading string data.
 *
 * 2. **Out-of-core workflows:** When the OOC import path builds the
 *    DataStructure skeleton, string arrays are initially populated with
 *    EmptyStringStore instances. A subsequent backfill step replaces each
 *    EmptyStringStore with a real StringStore that contains the loaded
 *    data.
 *
 * All data access methods (operator[], at, getValue, setValue, operator=)
 * throw std::runtime_error to fail fast if code accidentally tries to read
 * or write string data before the backfill step has run.
 *
 * @see StringStore The concrete store that holds real string data.
 * @see EmptyDataStore The equivalent placeholder for numeric DataArrays.
 */
class SIMPLNX_EXPORT EmptyStringStore : public AbstractStringStore
{
public:
  /**
   * @brief Default constructor.
   */
  EmptyStringStore() = default;

  /**
   * @brief Constructs an EmptyStringStore with the specified tuple shape.
   * @param tupleShape The shape of the tuple dimensions
   */
  EmptyStringStore(const ShapeType& tupleShape)
  : AbstractStringStore()
  , m_TupleShape(tupleShape)
  , m_NumTuples(std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<usize>(1), std::multiplies<>()))
  {
  }

  /**
   * @brief Copy constructor.
   * @param rhs The EmptyStringStore to copy from
   */
  EmptyStringStore(const EmptyStringStore& rhs) = default;

  /**
   * @brief Move constructor.
   * @param rhs The EmptyStringStore to move from
   */
  EmptyStringStore(EmptyStringStore&& rhs) = default;

  ~EmptyStringStore() override = default;

  /**
   * @brief Creates a deep copy of this EmptyStringStore.
   * @return std::unique_ptr<AbstractStringStore> Unique pointer to the deep copy
   */
  std::unique_ptr<AbstractStringStore> deepCopy() const override
  {
    return std::make_unique<EmptyStringStore>(*this);
  }

  /**
   * @brief Returns the total number of strings in the store (equal to the number of tuples).
   * @return usize The number of strings
   */
  usize size() const override
  {
    return m_NumTuples;
  }

  /**
   * @brief Returns whether the string store is empty.
   * @return bool True if the store has no strings, false otherwise
   */
  bool empty() const override
  {
    return m_NumTuples == 0;
  }

  /**
   * @brief Returns the number of tuples in the EmptyStringStore.
   * @return usize
   */
  usize getNumberOfTuples() const override
  {
    return m_NumTuples;
  }

  /**
   * @brief Returns the dimensions of the Tuples
   * @return
   */
  const ShapeType& getTupleShape() const override
  {
    return m_TupleShape;
  }

  /**
   * @brief Resizes the string store to the specified tuple shape.
   * @param tupleShape The new shape of the tuple dimensions
   */
  void resizeTuples(const ShapeType& tupleShape) override
  {
    m_TupleShape = tupleShape;
    m_NumTuples = std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<usize>(1), std::multiplies<>());
  }

  /**
   * @brief Returns true because EmptyStringStore is a metadata-only
   * placeholder that holds no actual string data.
   *
   * Code that needs to distinguish between a real StringStore (which has
   * accessible data) and an EmptyStringStore (which will throw on access)
   * should call isPlaceholder() rather than using dynamic_cast. This is
   * used by the backfill/import logic to identify which stores still need
   * their data loaded.
   *
   * @return true Always returns true for EmptyStringStore.
   */
  bool isPlaceholder() const override
  {
    return true;
  }

  /**
   * @brief Throws an error because EmptyStringStore has no data.
   * @param index The index (unused)
   * @throw std::runtime_error Always throws because EmptyStringStore has no data
   */
  reference operator[](usize index) override
  {
    throw std::runtime_error("EmptyStringStore::operator[] called on placeholder store - data not loaded yet");
  }

  /**
   * @brief Throws an error because EmptyStringStore has no data.
   * @param index The index (unused)
   * @throw std::runtime_error Always throws because EmptyStringStore has no data
   */
  const_reference operator[](usize index) const override
  {
    throw std::runtime_error("EmptyStringStore::operator[] called on placeholder store - data not loaded yet");
  }

  /**
   * @brief Throws an error because EmptyStringStore has no data.
   * @param index The index (unused)
   * @throw std::runtime_error Always throws because EmptyStringStore has no data
   */
  const_reference at(usize index) const override
  {
    throw std::runtime_error("EmptyStringStore::at() called on placeholder store - data not loaded yet");
  }

  /**
   * @brief Throws an error because EmptyStringStore has no data.
   * @param index The index (unused)
   * @throw std::runtime_error Always throws because EmptyStringStore has no data
   */
  const_reference getValue(usize index) const override
  {
    throw std::runtime_error("EmptyStringStore::getValue() called on placeholder store - data not loaded yet");
  }

  /**
   * @brief Throws an error because EmptyStringStore has no data.
   * @param index The index (unused)
   * @param value The value to set (unused)
   * @throw std::runtime_error Always throws because EmptyStringStore has no data
   */
  void setValue(usize index, const value_type& value) override
  {
    throw std::runtime_error("EmptyStringStore::setValue() called on placeholder store - data not loaded yet");
  }

  /**
   * @brief Throws an error because EmptyStringStore cannot accept data assignments.
   * @param values Vector of strings to assign (unused)
   * @throw std::runtime_error Always throws because EmptyStringStore has no data
   */
  AbstractStringStore& operator=(const std::vector<std::string>& values) override
  {
    throw std::runtime_error("EmptyStringStore::operator= called on placeholder store - data not loaded yet");
  }

private:
  ShapeType m_TupleShape;
  usize m_NumTuples = 0;
};
} // namespace nx::core
