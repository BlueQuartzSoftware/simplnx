#pragma once

#include "complex/DataStructure/AbstractDataStore.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DatasetReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

#include <fmt/format.h>

#include <numeric>
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
    return std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());
  }

  /**
   * @brief Returns the target tuple getSize.
   * @return usize
   */
  size_t getNumberOfComponents() const override
  {
    return std::accumulate(m_ComponentShape.cbegin(), m_ComponentShape.cend(), static_cast<size_t>(1), std::multiplies<>());
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
    return IDataStore::StoreType::Empty;
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

  /**
   * @brief Throws a runtime error due to the inability to write values to HDF5.
   * @param datasetWriter
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DatasetWriter& datasetWriter) const override
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Creates and returns an EmptyDataStore from the provided DatasetReader
   * @param datasetReader
   * @return std::unique_ptr<EmptyDataStore>
   */
  static std::unique_ptr<EmptyDataStore> ReadHdf5(const H5::DatasetReader& datasetReader)
  {
    // tupleShape
    H5::AttributeReader tupleShapeAttribute = datasetReader.getAttribute(IDataStore::k_TupleShape);
    if(!tupleShapeAttribute.isValid())
    {
      throw std::runtime_error(fmt::format("Error reading DataStore from HDF5 at {}/{}", H5::Support::GetObjectPath(datasetReader.getParentId()), datasetReader.getName()));
    }
    typename AbstractDataStore<T>::ShapeType tupleShape = tupleShapeAttribute.readAsVector<size_t>();

    // componentShape
    H5::AttributeReader componentShapeAttribute = datasetReader.getAttribute(AbstractDataStore<T>::k_ComponentShape);
    if(!componentShapeAttribute.isValid())
    {
      throw std::runtime_error(fmt::format("Error reading DataStore from HDF5 at {}/{}", H5::Support::GetObjectPath(datasetReader.getParentId()), datasetReader.getName()));
    }
    typename AbstractDataStore<T>::ShapeType componentShape = componentShapeAttribute.readAsVector<size_t>();

    // Create DataStore
    auto dataStore = std::make_unique<EmptyDataStore<T>>(tupleShape, componentShape);
    return dataStore;
  }

private:
  ShapeType m_ComponentShape;
  ShapeType m_TupleShape;
};
} // namespace complex
