#pragma once

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

  std::unique_ptr<AbstractStringStore> deepCopy() const override
  {
    return std::make_unique<EmptyStringStore>(*this);
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

  void resizeTuples(const ShapeType& tupleShape) override
  {
    m_TupleShape = tupleShape;
    m_NumTuples = std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<usize>(1), std::multiplies<>());
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
