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
    throw std::runtime_error("EmptyDataStore::resizeTuples() is not implemented");
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target getSize.
   * @param index
   * @return value_type
   */
  value_type getValue(usize index) const override
  {
    throw std::runtime_error("EmptyDataStore::getValue() is not implemented");
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target getSize.
   * @param index
   * @param value
   */
  void setValue(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::setValue() is not implemented");
  }

  /**
   * @brief Throws an exception because this should never be called.
   */
  void getValues(usize startIndex, nonstd::span<T> buffer) const override
  {
    throw std::runtime_error("EmptyDataStore::getValues() is not implemented");
  }

  /**
   * @brief Throws an exception because this should never be called.
   */
  void setValues(usize startIndex, nonstd::span<const T> buffer) override
  {
    throw std::runtime_error("EmptyDataStore::setValues() is not implemented");
  }

  /**
   * @brief Throws an exception because this should never be called. The
   * EmptyDataStore class contains no data other than its target getSize.
   * @param index
   * @return value_type
   */
  value_type at(usize index) const override
  {
    throw std::runtime_error("EmptyDataStore::at() is not implemented");
  }

  /**
   * @brief Adds value to value at index (equivalent to +=)
   * @param index
   * @param value
   */
  void add(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::add() is not implemented");
  }

  /**
   * @brief Subtracts value to value at index (equivalent to -=)
   * @param index
   * @param value
   */
  void sub(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::sub() is not implemented");
  }

  /**
   * @brief Multiplies value at index by value (equivalent to *=)
   * @param index
   * @param value
   */
  void mul(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::mul() is not implemented");
  }

  /**
   * @brief Divides value at index by value (equivalent to /=)
   * @param index
   * @param value
   */
  void div(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::div() is not implemented");
  }

  /**
   * @brief Takes remainder of value at index divided by value (equivalent to %=)
   * @param index
   * @param value
   */
  void rem(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::rem() is not implemented");
  }

  /**
   * @brief Bitwise AND of value at index with value (equivalent to &=)
   * @param index
   * @param value
   */
  void bitwiseAND(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::bitwiseAND() is not implemented");
  }

  /**
   * @brief Bitwise OR of value at index with value (equivalent to |=)
   * @param index
   * @param value
   */
  void bitwiseOR(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::bitwiseOR() is not implemented");
  }

  /**
   * @brief Bitwise XOR of value at index with value (equivalent to ^=)
   * @param index
   * @param value
   */
  void bitwiseXOR(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::bitwiseXOR() is not implemented");
  }

  /**
   * @brief Bitwise left shift of value at index with value (equivalent to <<=)
   * @param index
   * @param value
   */
  void bitwiseLShift(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::bitwiseLShift() is not implemented");
  }

  /**
   * @brief Bitwise right shift of value at index with value (equivalent to >>=)
   * @param index
   * @param value
   */
  void bitwiseRShift(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::bitwiseRShift() is not implemented");
  }

  /**
   * @brief Swaps bytes of value at index
   * @param index
   * @param value
   */
  void byteSwap(usize index) override
  {
    throw std::runtime_error("EmptyDataStore::byteSwap() is not implemented");
  }

  /**
   * @brief Swaps values at index1 and index2
   * @param index1
   * @param index2
   */
  void swap(usize index1, usize index2) override
  {
    throw std::runtime_error("EmptyDataStore::swap() is not implemented");
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
   * @brief Returns an error because EmptyDataStore cannot write binary files.
   * @param absoluteFilePath The file path (unused)
   * @return std::pair<int32, std::string> Error code and message
   */
  std::pair<int32, std::string> writeBinaryFile(const std::string& absoluteFilePath) const override
  {
    return {-10175, fmt::format("EmptyDataStore cannot read or write files", absoluteFilePath)};
  }

  /**
   * @brief Returns an error because EmptyDataStore cannot write binary files.
   * @param outputStream The output stream (unused)
   * @return std::pair<int32, std::string> Error code and message
   */
  std::pair<int32, std::string> writeBinaryFile(std::ostream& outputStream) const override
  {
    return {-10175, fmt::format("EmptyDataStore cannot read or write files")};
  }

  /**
   * @brief Returns an error because EmptyDataStore cannot read HDF5 data.
   * @param dataset The HDF5 dataset (unused)
   * @return Result<> Error result
   */
  Result<> readHdf5(const HDF5::DatasetIO& dataset) override
  {
    return MakeErrorResult(-42350, "Cannot read data into an EmptyDataStore");
  }

  /**
   * @brief Returns an error because EmptyDataStore cannot write HDF5 data.
   * @param dataset The HDF5 dataset (unused)
   * @return Result<> Error result
   */
  Result<> writeHdf5(HDF5::DatasetIO& dataset) const override
  {
    return MakeErrorResult(-42350, "Cannot write data from an EmptyDataStore");
  }

  /**
   * @brief Creates and returns an in-memory AbstractDataStore from a copy of the data
   * from the specified chunk.
   * @param flatChunkIndex
   */
  std::unique_ptr<AbstractDataStore<T>> convertChunkToDataStore(uint64 flatChunkIndex) const override
  {
    return nullptr;
  }

  /**
   * @brief Returns empty bounds because EmptyDataStore has no chunks.
   * @param flatChunkIndex The chunk index (unused)
   * @return ShapeType Empty shape vector
   */
  ShapeType getChunkLowerBounds(uint64 flatChunkIndex) const override
  {
    return {};
  }

  /**
   * @brief Returns empty bounds because EmptyDataStore has no chunks.
   * @param flatChunkIndex The chunk index (unused)
   * @return ShapeType Empty shape vector
   */
  ShapeType getChunkUpperBounds(uint64 flatChunkIndex) const override
  {
    return {};
  }

  /**
   * @brief Returns the number of chunks in the EmptyDataStore.
   * @return uint64 Always returns 0 because EmptyDataStore has no data
   */
  uint64 getNumberOfChunks() const override
  {
    return 0;
  }

private:
  ShapeType m_ComponentShape;
  ShapeType m_TupleShape;
  size_t m_NumComponents = {0};
  size_t m_NumTuples = {0};
  std::string m_DataFormat = "";
};
} // namespace nx::core
