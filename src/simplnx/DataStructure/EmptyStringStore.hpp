#pragma once

#include "simplnx/Utilities/StoreCopyUtilities.hpp"

#include "AbstractStringStore.hpp"

#include <functional>
#include <numeric>
#include <stdexcept>

namespace nx::core
{

/**
 * @class EmptyStringStore
 * @brief Stores StringArray tuple metadata without string values.
 *
 * Metadata-only import retains array shape without allocating each string.
 * Import finalization replaces this store with StringStore. Value access and
 * mutation throw until that replacement occurs.
 */
class SIMPLNX_EXPORT EmptyStringStore : public AbstractStringStore
{
public:
  EmptyStringStore() = default;

  EmptyStringStore(const ShapeType& tupleShape)
  : AbstractStringStore()
  , m_TupleShape(tupleShape)
  , m_NumTuples(std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<usize>(1), std::multiplies<>()))
  {
  }

  EmptyStringStore(const EmptyStringStore& rhs) = default;
  EmptyStringStore(EmptyStringStore&& rhs) = default;
  ~EmptyStringStore() override = default;

  /**
   * @brief Copies string shape metadata without allocating values.
   * @param destinationFormat Resolved destination format; empty and the canonical in-memory name select memory.
   * @return Independent metadata placeholder with the same shape and no values.
   * @throws std::runtime_error If metadata validation fails or an OOC build rejects an unavailable format.
   * @throws std::bad_alloc If a metadata allocation fails.
   *
   * In-core builds use the in-memory selection for unavailable formats. OOC builds reject unavailable formats.
   * This placeholder has no planned-format field. Format validation does not allocate a destination value store.
   * This store validates destinationFormat and does not resolve storage policy itself.
   * Resolve destination policy before this call. Use the destination DataStructure for automatic policy selection.
   */
  std::unique_ptr<AbstractStringStore> deepCopy(const std::string& destinationFormat) const override
  {
    return CopyStringStore(*this, destinationFormat);
  }

  usize size() const override
  {
    return m_NumTuples;
  }

  bool empty() const override
  {
    return m_NumTuples == 0;
  }

  usize getNumberOfTuples() const override
  {
    return m_NumTuples;
  }

  const ShapeType& getTupleShape() const override
  {
    return m_TupleShape;
  }

  /**
   * @brief Changes the placeholder tuple shape without accessing values.
   * @param tupleShape New tuple dimensions in slowest-to-fastest order.
   * @return Always valid because preflight placeholders contain no values.
   *
   * Preflight must resize metadata before execution materializes the string store.
   */
  [[nodiscard]] Result<> resizeTuples(const ShapeType& tupleShape) override
  {
    m_TupleShape = tupleShape;
    m_NumTuples = std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<usize>(1), std::multiplies<>());
    return {};
  }

  bool isPlaceholder() const override
  {
    return true;
  }

  reference operator[](usize index) override
  {
    throw std::runtime_error("EmptyStringStore::operator[] called on placeholder store - data not loaded yet");
  }

  const_reference operator[](usize index) const override
  {
    throw std::runtime_error("EmptyStringStore::operator[] called on placeholder store - data not loaded yet");
  }

  const_reference at(usize index) const override
  {
    throw std::runtime_error("EmptyStringStore::at() called on placeholder store - data not loaded yet");
  }

  const_reference getValue(usize index) const override
  {
    throw std::runtime_error("EmptyStringStore::getValue() called on placeholder store - data not loaded yet");
  }

  void setValue(usize index, const value_type& value) override
  {
    throw std::runtime_error("EmptyStringStore::setValue() called on placeholder store - data not loaded yet");
  }

  AbstractStringStore& operator=(const std::vector<std::string>& values) override
  {
    throw std::runtime_error("EmptyStringStore::operator= called on placeholder store - data not loaded yet");
  }

private:
  ShapeType m_TupleShape;
  usize m_NumTuples = 0;
};
} // namespace nx::core
