#pragma once

#include "simplnx/DataStructure/AbstractDataStore.hpp"

#include <fmt/format.h>

#include <numeric>
#include <stdexcept>
#include <vector>

namespace nx::core
{
/**
 * @class EmptyDataStore
 * @brief The EmptyDataStore class serves as a placeholder IDataStore for use
 * in preflight where data is not available but the number and getSize of tuples
 * is known.
 * @tparam T
 */
template <typename T>
class EmptyDataStore : public AbstractDataStore<T>
{
public:
  using value_type = typename AbstractDataStore<T>::value_type;
  using reference = typename AbstractDataStore<T>::reference;
  using const_reference = typename AbstractDataStore<T>::const_reference;
  using ShapeType = typename IDataStore::ShapeType;

  /**
   * @brief Constructs an empty data store with a tuple getSize and count of 0.
   */
  EmptyDataStore() = default;

  /**
   * @brief Constructs an empty data store with the specified tupleSize and tupleCount.
   * @param tupleSize
   * @param tupleCount
   * @param inMemory Stores whether or not the created data will be kept in memory or handled out of core
   */
  EmptyDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::string dataFormat = "")
  : m_ComponentShape(componentShape)
  , m_TupleShape(tupleShape)
  , m_NumComponents(std::accumulate(m_ComponentShape.cbegin(), m_ComponentShape.cend(), static_cast<size_t>(1), std::multiplies<>()))
  , m_NumTuples(std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<size_t>(1), std::multiplies<>()))
  , m_DataFormat(dataFormat)
  {
  }

  /**
   * @brief Copy constructor
   * @param other
   */
  EmptyDataStore(const EmptyDataStore& other)
  : m_ComponentShape(other.m_ComponentShape)
  , m_TupleShape(other.m_TupleShape)
  , m_NumComponents(other.m_NumComponents)
  , m_NumTuples(other.m_NumTuples)
  , m_DataFormat(other.m_DataFormat)
  {
  }

  /**
   * @brief Move constructor
   * @param other
   */
  EmptyDataStore(EmptyDataStore&& other) noexcept
  : m_ComponentShape(std::move(other.m_ComponentShape))
  , m_TupleShape(std::move(other.m_TupleShape))
  , m_NumComponents(std::move(other.m_NumComponents))
  , m_NumTuples(std::move(other.m_NumTuples))
  , m_DataFormat(other.m_DataFormat)
  {
  }

  ~EmptyDataStore() override = default;

  /**
   * @brief Returns the number of tuples that should be in the data store.
   * @return usize
   */
  usize getNumberOfTuples() const override
  {
    return m_NumTuples;
  }

  /**
   * @brief Returns the target tuple getSize.
   * @return usize
   */
  size_t getNumberOfComponents() const override
  {
    return m_NumComponents;
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
   * @brief Returns the dimensions of the Components
   * @return
   */
  const ShapeType& getComponentShape() const override
  {
    return m_ComponentShape;
  }

  /**
   * @brief Returns the store type e.g. in memory, out of core, etc.
   * @return StoreType
   */
  IDataStore::StoreType getStoreType() const override
  {
    return m_DataFormat.empty() ? IDataStore::StoreType::Empty : IDataStore::StoreType::EmptyOutOfCore;
  }

  /**
   * @brief Checks and returns if the created data store should be in memory or handled out of core.
   * @return bool
   */
  std::string dataFormat() const
  {
    return m_DataFormat;
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target size.
   * @param tupleShape
   */
  void resizeTuples(const ShapeType& tupleShape) override
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
  const_reference at(usize index) const override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target getSize.
   * @param  index
   * @return const_reference
   */
  const_reference operator[](usize index) const override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target getSize.
   * @param  index
   * @return reference
   */
  reference operator[](usize index) override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Returns a deep copy of the data store and all its data.
   * @return std::unique_ptr<IDataStore>
   */
  std::unique_ptr<IDataStore> deepCopy() const override
  {
    return std::make_unique<EmptyDataStore>(*this);
  }

  /**
   * @brief Returns a data store of the same type as this but with default initialized data.
   * @return std::unique_ptr<IDataStore>
   */
  std::unique_ptr<IDataStore> createNewInstance() const override
  {
    return std::make_unique<EmptyDataStore<T>>(this->getTupleShape(), this->getComponentShape());
  }

  std::pair<int32, std::string> writeBinaryFile(const std::string& absoluteFilePath) const override
  {
    return {-10175, fmt::format("EmptyDataStore cannot read or write files", absoluteFilePath)};
  }
  std::pair<int32, std::string> writeBinaryFile(std::ostream& outputStream) const override
  {
    return {-10175, fmt::format("EmptyDataStore cannot read or write files")};
  }

private:
  ShapeType m_ComponentShape;
  ShapeType m_TupleShape;
  size_t m_NumComponents = {0};
  size_t m_NumTuples = {0};
  std::string m_DataFormat = "";
};
} // namespace nx::core
