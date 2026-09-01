#pragma once

#include "simplnx/DataStructure/AbstractDataStore.hpp"

#include <fmt/format.h>

#include <map>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */
namespace nx::core
{
/**
 * @class EmptyDataStore
 * @brief Preserves data-store metadata without allocating values.
 * @tparam T Planned value type.
 */
template <typename T>
class EmptyDataStore : public AbstractDataStore<T>
{
public:
  /**
   * @brief Names the planned value type.
   */
  using value_type = typename AbstractDataStore<T>::value_type;

  /**
   * @brief Names the mutable value-proxy type.
   */
  using reference = typename AbstractDataStore<T>::reference;

  /**
   * @brief Creates an empty metadata store.
   */
  EmptyDataStore() = default;

  /**
   * @brief Creates a metadata store with planned shapes and format.
   * @param tupleShape Planned tuple dimensions in slowest-to-fastest order.
   * @param componentShape Planned component dimensions in slowest-to-fastest order.
   * @param dataFormat Planned out-of-core format, or empty for in-memory storage.
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
   * @brief Copies metadata-store state.
   * @param other Source metadata store.
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
   * @brief Moves metadata-store state.
   * @param other Source metadata store.
   */
  EmptyDataStore(EmptyDataStore&& other) noexcept
  : m_ComponentShape(std::move(other.m_ComponentShape))
  , m_TupleShape(std::move(other.m_TupleShape))
  , m_NumComponents(std::move(other.m_NumComponents))
  , m_NumTuples(std::move(other.m_NumTuples))
  , m_DataFormat(other.m_DataFormat)
  {
  }

  /**
   * @brief Destroys the metadata store.
   */
  ~EmptyDataStore() override = default;

  usize getNumberOfTuples() const override
  {
    return m_NumTuples;
  }

  usize getNumberOfComponents() const override
  {
    return m_NumComponents;
  }

  const ShapeType& getTupleShape() const override
  {
    return m_TupleShape;
  }

  const ShapeType& getComponentShape() const override
  {
    return m_ComponentShape;
  }

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::Empty;
  }

  /**
   * @brief Returns the store type that materializes after preflight.
   *
   * An empty planned format selects in-memory storage. A non-empty format
   * selects out-of-core storage without allocating values.
   * @return Planned in-memory or out-of-core store type.
   */
  IDataStore::StoreType getPlannedStoreType() const override
  {
    return m_DataFormat.empty() ? IDataStore::StoreType::InMemory : IDataStore::StoreType::OutOfCore;
  }

  /**
   * @brief Rejects recovery metadata access.
   * @return Does not return.
   * @throws std::runtime_error Always, because this store has no backing data.
   */
  std::map<std::string, std::string> getRecoveryMetadata() const override
  {
    throw std::runtime_error("EmptyDataStore::getRecoveryMetadata: cannot query recovery metadata on a placeholder store");
  }

  /**
   * @brief Returns the planned storage format.
   * @return Empty string for in-memory storage, or an out-of-core format name.
   *
   * getDataFormat() remains empty. Output actions use it to select execution
   * storage, while this method exposes preflight planning without changing that selection.
   */
  std::string dataFormat() const
  {
    return m_DataFormat;
  }

  /**
   * @brief Returns planned in-memory usage in bytes.
   * @return Logical byte size for in-memory storage, or zero for out-of-core storage.
   *
   * getDataFormat() intentionally stays empty so preflight output actions keep
   * their execution storage selection. dataFormat() exposes the planned format.
   */
  uint64 memoryUsage() const override
  {
    return m_DataFormat.empty() ? (sizeof(T) * this->getSize()) : 0;
  }

  /**
   * @brief Rejects tuple-shape changes.
   * @param tupleShape Requested tuple shape.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void resizeTuples(const ShapeType& tupleShape) override
  {
    throw std::runtime_error("EmptyDataStore::resizeTuples() is not implemented");
  }

  /**
   * @brief Rejects value access.
   * @param index Flat value index.
   * @return Does not return.
   * @throws std::runtime_error Always, because this store has no values.
   */
  value_type getValue(usize index) const override
  {
    throw std::runtime_error("EmptyDataStore::getValue() is not implemented");
  }

  /**
   * @brief Rejects value writes.
   * @param index Flat value index.
   * @param value Value to store.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void setValue(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::setValue() is not implemented");
  }

  /**
   * @brief Rejects bulk reads.
   * @param startIndex First requested flat value index.
   * @param buffer Destination buffer.
   * @return Error because this store has no values.
   */
  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    return MakeErrorResult(-6022, "EmptyDataStore bulk read is not supported: EmptyDataStore is a metadata-only placeholder used during preflight and must be replaced with a real DataStore or "
                                  "out-of-core store before bulk I/O is attempted.");
  }

  /**
   * @brief Rejects bulk writes.
   * @param startIndex First requested flat value index.
   * @param buffer Source buffer.
   * @return Error because this store has no values.
   */
  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    return MakeErrorResult(-6023, "EmptyDataStore bulk write is not supported: EmptyDataStore is a metadata-only placeholder used during preflight and must be replaced with a real DataStore or "
                                  "out-of-core store before bulk I/O is attempted.");
  }

  /**
   * @brief Returns no extent values.
   * @param extent Requested tuple-space extent.
   * @return Empty value vector because this store has no values.
   */
  std::vector<T> readExtent(const Extent& extent) const override
  {
    return {};
  }

  /**
   * @brief Rejects caller-buffer extent reads.
   * @param extent Requested tuple-space extent.
   * @param destination Destination buffer.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void readExtentIntoBuffer(const Extent& extent, nonstd::span<T> destination) const override
  {
    (void)extent;
    (void)destination;
    throw std::runtime_error("EmptyDataStore::readExtentIntoBuffer is not supported: EmptyDataStore is a metadata-only preflight placeholder");
  }

  /**
   * @brief Ignores extent writes.
   * @param extent Requested tuple-space extent.
   * @param data Source values.
   *
   * Preflight writes have no values to modify. Execution replaces this store
   * before meaningful data access.
   */
  void writeExtent(const Extent& extent, nonstd::span<const T> data) override
  {
    // Preflight metadata stores do not retain values.
  }

  /**
   * @brief Rejects bounds-checked value access.
   * @param index Flat value index.
   * @return Does not return.
   * @throws std::runtime_error Always, because this store has no values.
   */
  value_type at(usize index) const override
  {
    throw std::runtime_error("EmptyDataStore::at() is not implemented");
  }

  /**
   * @brief Rejects value addition.
   * @param index Flat value index.
   * @param value Value to add.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void add(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::add() is not implemented");
  }

  /**
   * @brief Rejects value subtraction.
   * @param index Flat value index.
   * @param value Value to subtract.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void sub(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::sub() is not implemented");
  }

  /**
   * @brief Rejects value multiplication.
   * @param index Flat value index.
   * @param value Multiplier.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void mul(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::mul() is not implemented");
  }

  /**
   * @brief Rejects value division.
   * @param index Flat value index.
   * @param value Divisor.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void div(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::div() is not implemented");
  }

  /**
   * @brief Rejects remainder operations.
   * @param index Flat value index.
   * @param value Divisor.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void rem(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::rem() is not implemented");
  }

  /**
   * @brief Rejects bitwise AND operations.
   * @param index Flat value index.
   * @param value Operand.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void bitwiseAND(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::bitwiseAND() is not implemented");
  }

  /**
   * @brief Rejects bitwise OR operations.
   * @param index Flat value index.
   * @param value Operand.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void bitwiseOR(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::bitwiseOR() is not implemented");
  }

  /**
   * @brief Rejects bitwise XOR operations.
   * @param index Flat value index.
   * @param value Operand.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void bitwiseXOR(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::bitwiseXOR() is not implemented");
  }

  /**
   * @brief Rejects left-shift operations.
   * @param index Flat value index.
   * @param value Shift count.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void bitwiseLShift(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::bitwiseLShift() is not implemented");
  }

  /**
   * @brief Rejects right-shift operations.
   * @param index Flat value index.
   * @param value Shift count.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void bitwiseRShift(usize index, value_type value) override
  {
    throw std::runtime_error("EmptyDataStore::bitwiseRShift() is not implemented");
  }

  /**
   * @brief Rejects byte-order changes.
   * @param index Flat value index.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void byteSwap(usize index) override
  {
    throw std::runtime_error("EmptyDataStore::byteSwap() is not implemented");
  }

  /**
   * @brief Rejects value swaps.
   * @param index1 First flat value index.
   * @param index2 Second flat value index.
   * @throws std::runtime_error Always, because this store has no values.
   */
  void swap(usize index1, usize index2) override
  {
    throw std::runtime_error("EmptyDataStore::swap() is not implemented");
  }

  /**
   * @brief Makes an independent metadata-store copy.
   * @return Owning copy of this metadata store.
   */
  std::unique_ptr<IDataStore> deepCopy() const override
  {
    return std::make_unique<EmptyDataStore>(*this);
  }

  /**
   * @brief Creates a metadata store with the same shapes.
   * @return Owning metadata store.
   */
  std::unique_ptr<IDataStore> createNewInstance() const override
  {
    return std::make_unique<EmptyDataStore<T>>(this->getTupleShape(), this->getComponentShape());
  }

  /**
   * @brief Rejects binary-file writes.
   * @param absoluteFilePath Destination file path.
   * @return Error code and message because this store has no values.
   */
  std::pair<int32, std::string> writeBinaryFile(const std::string& absoluteFilePath) const override
  {
    return {-10175, fmt::format("EmptyDataStore cannot read or write files", absoluteFilePath)};
  }

  /**
   * @brief Rejects binary-stream writes.
   * @param outputStream Destination stream.
   * @return Error code and message because this store has no values.
   */
  std::pair<int32, std::string> writeBinaryFile(std::ostream& outputStream) const override
  {
    return {-10175, fmt::format("EmptyDataStore cannot read or write files")};
  }

  /**
   * @brief Rejects HDF5 reads.
   * @param dataset HDF5 dataset to read.
   * @return Error because this store has no values.
   */
  Result<> readHdf5(const HDF5::DatasetIO& dataset) override
  {
    return MakeErrorResult(-42350, "Cannot read data into an EmptyDataStore");
  }

  /**
   * @brief Rejects HDF5 writes.
   * @param dataset HDF5 dataset to write.
   * @return Error because this store has no values.
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
