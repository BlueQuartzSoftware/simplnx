#pragma once

#include "simplnx/DataStructure/AbstractDataStore.hpp"

#include <fmt/format.h>

#include <map>
#include <numeric>
#include <stdexcept>
#include <string>
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
  , m_NumComponents(std::accumulate(m_ComponentShape.cbegin(), m_ComponentShape.cend(), static_cast<usize>(1), std::multiplies<>()))
  , m_NumTuples(std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<usize>(1), std::multiplies<>()))
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
  usize getNumberOfComponents() const override
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
   * @brief Returns StoreType::Empty because this store is a metadata-only
   * placeholder. The dataFormat() string records the intended storage
   * strategy (e.g., "" for in-memory, or a named OOC format) so the
   * framework knows what real store to create when execution begins.
   * @return StoreType::Empty
   */
  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::Empty;
  }

  /**
   * @brief Throws — EmptyDataStore is a metadata-only placeholder.
   *
   * EmptyDataStore holds no data and no backing file, so it has no
   * recovery metadata to report. Calling getRecoveryMetadata() on an
   * EmptyDataStore is a programming error: the caller is treating a
   * placeholder as if it were a real store. The real store that
   * replaces this placeholder during execution is the one responsible
   * for providing recovery metadata.
   *
   * Throws std::runtime_error to fail fast, matching the behavior of
   * the other data-access methods on this class.
   */
  std::map<std::string, std::string> getRecoveryMetadata() const override
  {
    throw std::runtime_error("EmptyDataStore::getRecoveryMetadata: cannot query recovery metadata on a placeholder store");
  }

  /**
   * @brief Returns the data format string that was specified at construction.
   *
   * This string indicates the intended storage strategy for the real data
   * store that will replace this EmptyDataStore after preflight:
   * - An empty string ("") means the data will be stored in-memory (DataStore).
   * - A non-empty string names an out-of-core format (e.g., an OOC store
   *   implementation) that should be used for execution.
   *
   * @return std::string The data format identifier
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
   * @brief Always returns an invalid Result because EmptyDataStore holds no
   * data. EmptyDataStore is a metadata-only placeholder used during preflight;
   * bulk data access is not supported. The store must be replaced with a real
   * DataStore or OOC store before any data I/O is attempted.
   * @param startIndex Unused
   * @param buffer Unused
   * @return Invalid Result<> — always.
   */
  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    return MakeErrorResult(-6022, "EmptyDataStore bulk read is not supported: EmptyDataStore is a metadata-only placeholder used during preflight and must be replaced with a real DataStore or "
                                  "out-of-core store before bulk I/O is attempted.");
  }

  /**
   * @brief Always returns an invalid Result because EmptyDataStore holds no
   * data. EmptyDataStore is a metadata-only placeholder used during preflight;
   * bulk data access is not supported. The store must be replaced with a real
   * DataStore or OOC store before any data I/O is attempted.
   * @param startIndex Unused
   * @param buffer Unused
   * @return Invalid Result<> — always.
   */
  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    return MakeErrorResult(-6023, "EmptyDataStore bulk write is not supported: EmptyDataStore is a metadata-only placeholder used during preflight and must be replaced with a real DataStore or "
                                  "out-of-core store before bulk I/O is attempted.");
  }

  /**
   * @brief Returns an empty vector because EmptyDataStore holds no data.
   * EmptyDataStore is a metadata-only placeholder used during preflight and
   * bulk data access is not supported — the store must be replaced with a
   * real DataStore or out-of-core store before extent reads are attempted.
   * @param extent Unused
   * @return Empty std::vector<T>
   */
  std::vector<T> readExtent(const Extent& extent) const override
  {
    return {};
  }

  /**
   * @brief No-op because EmptyDataStore holds no data. Extent writes against
   * an EmptyDataStore are silently dropped; the store must be replaced with
   * a real DataStore or out-of-core store before meaningful writes are
   * attempted.
   * @param extent Unused
   * @param data Unused
   */
  void writeExtent(const Extent& extent, nonstd::span<const T> data) override
  {
    // No-op: EmptyDataStore is a metadata-only placeholder.
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

private:
  ShapeType m_ComponentShape;
  ShapeType m_TupleShape;
  usize m_NumComponents = {0};
  usize m_NumTuples = {0};
  std::string m_DataFormat = "";
};
} // namespace nx::core
